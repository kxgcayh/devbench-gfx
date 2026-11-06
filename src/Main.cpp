#include "DevBenchAPI.h"
#include "Path.h"

#include <chrono>
#include <future>
#include <memory>

using json = nlohmann::json;

namespace
{
    constexpr int kDefaultDepth = 2;
    constexpr int kMaxDepth = 8;
    constexpr std::uint32_t kDefaultLimit = 500;
    constexpr int kMaxNodes = 20000;  // display objects link back to parents, so depth alone is not a budget
    constexpr auto kUiTimeout = 3s;

    json Error(const std::string& a_message)
    {
        return { { "ok", false }, { "error", a_message } };
    }

    json ToJson(const RE::GFxValue& a_value, int a_depth, std::uint32_t a_limit, int& a_nodes)
    {
        using Type = RE::GFxValue::ValueType;
        if (++a_nodes > kMaxNodes) {
            return "<node limit>";
        }
        switch (a_value.GetType()) {
        case Type::kBoolean:
            return a_value.GetBool();
        case Type::kNumber:
            return a_value.GetNumber();
        case Type::kString:
            return a_value.GetString() ? a_value.GetString() : "";
        case Type::kArray:
            {
                const auto size = a_value.GetArraySize();
                if (a_depth <= 0) {
                    return std::format("<array {}>", size);
                }
                json out = json::array();
                for (std::uint32_t i = 0; i < size && i < a_limit; ++i) {
                    RE::GFxValue item;
                    a_value.GetElement(i, &item);
                    out.push_back(ToJson(item, a_depth - 1, a_limit, a_nodes));
                }
                return out;
            }
        case Type::kObject:
        case Type::kDisplayObject:
            {
                if (a_depth <= 0) {
                    return "<object>";
                }
                json out = json::object();
                // This Scaleform has no function type: methods arrive as objects and print as {}.
                a_value.VisitMembers([&](const char* a_name, const RE::GFxValue& a_member) {
                    out[a_name] = ToJson(a_member, a_depth - 1, a_limit, a_nodes);
                });
                return out;
            }
        default:
            return nullptr;  // undefined and null
        }
    }

    void FromJson(RE::GFxMovieView* a_movie, const json& a_value, RE::GFxValue& a_out)
    {
        if (a_value.is_boolean()) {
            a_out.SetBoolean(a_value.get<bool>());
        } else if (a_value.is_number()) {
            a_out.SetNumber(a_value.get<double>());
        } else if (a_value.is_string()) {
            a_movie->CreateString(&a_out, a_value.get_ref<const std::string&>().c_str());
        } else if (a_value.is_array()) {
            a_movie->CreateArray(&a_out);
            for (const auto& item : a_value) {
                RE::GFxValue element;
                FromJson(a_movie, item, element);
                a_out.PushBack(element);
            }
        } else {
            a_out.SetNull();
        }
    }

    bool Step(const RE::GFxValue& a_from, const std::string& a_seg, RE::GFxValue& a_to)
    {
        // GFxValue only asserts its type, so a member lookup on a number or string dereferences garbage in release.
        if (!a_from.IsObject()) {
            return false;
        }
        if (a_from.IsArray() && IsIndex(a_seg)) {
            return a_from.GetElement(static_cast<std::uint32_t>(std::stoul(a_seg)), &a_to);
        }
        return a_from.GetMember(a_seg.c_str(), &a_to);
    }

    // Walks a_count segments. The first is a root the movie resolves itself: _root, _global, or a timeline variable.
    std::optional<std::string> Resolve(RE::GFxMovieView* a_movie, const std::vector<std::string>& a_segs,
        std::size_t a_count, RE::GFxValue& a_out)
    {
        if (!a_movie->GetVariable(&a_out, a_segs[0].c_str())) {
            return "root not found: " + a_segs[0];
        }
        for (std::size_t i = 1; i < a_count; ++i) {
            RE::GFxValue next;
            if (!Step(a_out, a_segs[i], next)) {
                return "member not found: " + a_segs[i];
            }
            a_out = next;
        }
        return std::nullopt;
    }

    json Run(const json& a_args)
    {
        const auto op = a_args.value("op", std::string{});
        const auto menuName = a_args.value("menu", std::string{});
        const auto segs = SplitPath(a_args.value("path", std::string{}));
        if (op != "get" && op != "set" && op != "call") {
            return Error("op must be get, set or call");
        }
        if (menuName.empty() || segs.empty()) {
            return Error("menu and path are required");
        }

        auto* ui = RE::UI::GetSingleton();
        if (!ui) {
            return Error("UI not ready");
        }
        const auto menu = ui->GetMenu(menuName);
        if (!menu || !menu->uiMovie) {
            return Error("menu not open: " + menuName);
        }
        auto* movie = menu->uiMovie.get();
        int nodes = 0;

        if (op == "get") {
            RE::GFxValue value;
            if (const auto error = Resolve(movie, segs, segs.size(), value)) {
                return Error(*error);
            }
            const int depth = std::clamp(a_args.value("depth", kDefaultDepth), 0, kMaxDepth);
            const auto limit = a_args.value("limit", kDefaultLimit);
            return { { "ok", true }, { "value", ToJson(value, depth, limit, nodes) }, { "nodes", nodes } };
        }

        if (segs.size() < 2) {
            return Error("set and call need a path with at least two segments");
        }
        RE::GFxValue parent;
        if (const auto error = Resolve(movie, segs, segs.size() - 1, parent)) {
            return Error(*error);
        }
        if (!parent.IsObject()) {
            return Error("not an object: " + segs[segs.size() - 2]);
        }
        const auto& leaf = segs.back();

        if (op == "set") {
            if (!a_args.contains("value")) {
                return Error("set needs a value");
            }
            RE::GFxValue value;
            FromJson(movie, a_args["value"], value);
            const bool done = parent.IsArray() && IsIndex(leaf) ?
                                  parent.SetElement(static_cast<std::uint32_t>(std::stoul(leaf)), value) :
                                  parent.SetMember(leaf.c_str(), value);
            return done ? json{ { "ok", true } } : Error("set failed: " + leaf);
        }

        std::vector<RE::GFxValue> args;
        for (const auto& arg : a_args.value("args", json::array())) {
            FromJson(movie, arg, args.emplace_back());
        }
        RE::GFxValue result;
        if (!parent.Invoke(leaf.c_str(), &result, args.data(), args.size())) {
            return Error("call failed: " + leaf);
        }
        return { { "ok", true }, { "value", ToJson(result, kDefaultDepth, kDefaultLimit, nodes) } };
    }

    // DevBench calls this on its listener thread, and Scaleform belongs to the UI thread.
    void Handle(void*, const char* a_argsJson, void* a_sink, DevBenchAPI::WriteFn a_write)
    {
        json args = json::parse(a_argsJson ? a_argsJson : "{}", nullptr, false);
        if (args.is_discarded()) {
            a_write(a_sink, Error("arguments are not valid JSON").dump().c_str());
            return;
        }

        auto done = std::make_shared<std::promise<std::string>>();
        auto result = done->get_future();
        SKSE::GetTaskInterface()->AddUITask([done, args = std::move(args)] {
            try {
                done->set_value(Run(args).dump(-1, ' ', false, json::error_handler_t::replace));
            } catch (const std::exception& e) {
                done->set_value(Error(e.what()).dump(-1, ' ', false, json::error_handler_t::replace));
            }
        });

        const auto text = result.wait_for(kUiTimeout) == std::future_status::ready ?
                              result.get() :
                              Error("timed out waiting for the UI thread").dump();
        a_write(a_sink, text.c_str());
    }

    constexpr auto kDescriptor = R"({"description":"Read, set or call anything in an open Scaleform menu. Args: op (get|set|call), menu (for example 'RaceSex Menu'), path (dotted, from _root or _global; numbers index arrays), plus depth and limit for get, value for set, args for call."})";

    void OnMessage(SKSE::MessagingInterface::Message* a_msg)
    {
        if (a_msg->type != SKSE::MessagingInterface::kDataLoaded) {
            return;
        }
        auto* api = DevBenchAPI::GetDevBenchInterface001();
        if (!api) {
            logger::warn("DevBench not found; nothing registered");
        } else if (api->GetBuildNumber() < 10500) {
            logger::warn("DevBench {} is older than 1.5.0; nothing registered", api->GetBuildNumber());
        } else {
            api->RegisterToolExtension("menu", "gfx", kDescriptor, &Handle, nullptr);
            logger::info("registered menu invoke name=gfx on DevBench {}", api->GetBuildNumber());
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    logger::info("DevBenchGFx loading on Skyrim {}", REL::Module::get().version().string());
    if (!SKSE::GetMessagingInterface()->RegisterListener(OnMessage)) {
        logger::error("failed to register messaging listener");
        return false;
    }
    return true;
}
