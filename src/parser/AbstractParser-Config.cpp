// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../configuration/NamedConfig.h"
#include "../configuration/configuration.h"
#include "../display/MapCanvasConfig.h"
#include "../display/MapCanvasData.h"
#include "../display/mapcanvas.h"
#include "../global/AnsiOstream.h"
#include "../global/Consts.h"
#include "../global/NamedColors.h"
#include "../global/PrintUtils.h"
#include "../mpi/remoteeditwidget.h"
#include "../proxy/proxy.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "AbstractParser-Utils.h"
#include "abstractparser.h"

#include <ostream>

#include <QColor>
#include <QDir>
#include <QFile>
#include <QPointer>
#include <QSettings>
#include <QTemporaryFile>

class NODISCARD ArgNamedColor final : public syntax::IArgument
{
private:
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const final;

    std::ostream &virt_to_stream(std::ostream &os) const final;
};

syntax::MatchResult ArgNamedColor::virt_match(const syntax::ParserInput &input,
                                              syntax::IMatchErrorLogger *) const
{
    if (input.empty()) {
        return syntax::MatchResult::failure(input);
    }

    auto arg = std::string_view{input.front()};

    auto names = XNamedColor::getAllNames();
    for (const auto &name : names) {
        if (name == arg) {
            return syntax::MatchResult::success(1, input, Value{name});
        }
    }

    return syntax::MatchResult::failure(input);
}

std::ostream &ArgNamedColor::virt_to_stream(std::ostream &os) const
{
    return os << "<NamedColor>";
}

class NODISCARD BoolAlpha final
{
private:
    bool m_value = false;

public:
    explicit BoolAlpha(const bool value)
        : m_value{value}
    {}

    friend std::ostream &operator<<(std::ostream &os, const BoolAlpha &x)
    {
        return os << (x.m_value ? "true" : "false");
    }
    friend AnsiOstream &operator<<(AnsiOstream &os, const BoolAlpha &x)
    {
        return os << (x.m_value ? "true" : "false");
    }
};

template<typename T>
inline decltype(auto) remap(T &&x)
{
    static_assert(!std::is_same_v<std::decay_t<T>, std::string_view>);
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>
                  || std::is_same_v<std::decay_t<T>, const char *>) {
        return syntax::abbrevToken(std::forward<T>(x));
    } else if constexpr (true) {
        return std::forward<T>(x);
    }

    std::abort();
}

// REVISIT: Can we (ab)use tuple to remap std::string to syntax::abbrevToken?
template<typename... Args>
NODISCARD static auto syn(Args &&...args)
{
    return syntax::buildSyntax(remap(std::forward<Args>(args))...);
}

void AbstractParser::doConfig(const StringView cmd)
{
    auto listColors = syntax::Accept(
        [](User &user, const Pair *) {
            auto &os = user.getOstream();

            auto names = XNamedColor::getAllNames();
            std::sort(names.begin(), names.end());

            os << "Customizable colors:\n";
            for (const auto &name : names) {
                if (name.empty() || name.front() == char_consts::C_PERIOD) {
                    continue;
                }
                os << " " << SmartQuotedString{name} << " = ";
                if (auto opt = XNamedColor::lookup(name)) {
                    const XNamedColor &color = *opt;
                    os << color.getColor();
                } else {
                    os << "(error)";
                }
                os << AnsiOstream::endl;
            }
        },
        "list colors");

    auto setNamedColor = syntax::Accept(
        [this](User &user, const Pair *args) {
            auto &os = user.getOstream();
            if (args == nullptr || args->cdr == nullptr || !args->car.isLong()
                || !args->cdr->car.isString()) {
                throw std::runtime_error("internal error");
            }

            const std::string name = args->cdr->car.getString();
            const auto rgb = static_cast<uint32_t>(args->car.getLong());

            auto opt = XNamedColor::lookup(name);
            if (!opt.has_value()) {
                throw std::runtime_error("invalid name: " + name);
            }

            auto &color = deref(opt);
            const auto oldColor = color.getColor();
            const auto newColor = Color::fromRGB(rgb);

            if (oldColor.getRGB() == rgb) {
                os << "Color " << SmartQuotedString{name} << " is already " << color.getColor()
                   << ".\n";
                return;
            }

            if (!color.setColor(newColor)) {
                os << "Color " << SmartQuotedString{name} << " cannot be changed from "
                   << color.getColor() << ".\n";
                return;
            }

            os << "Color " << SmartQuotedString{name} << " has been changed from " << oldColor
               << " to " << color.getColor() << ".\n";

            // FIXME: Some of the colors still require a map update.
            if ((false)) {
                graphicsSettingsChanged();
            } else {
                mapChanged();
            }
        },
        "set named color");

    auto makeSetFixedPoint = [this](FixedPoint<1> &fp, const std::string &help) -> syntax::Accept {
        //
        return syntax::Accept(
            [this, &fp, help](User &user, const Pair *const args) -> void {
                auto &os = user.getOstream();

                if (args == nullptr || !args->car.isFloat()) {
                    throw std::runtime_error("internal type error");
                }

                const float value = args->car.getFloat();
                const auto min = fp.clone(fp.min).getFloat();
                const auto max = fp.clone(fp.max).getFloat();
                if (value < min || value > max) {
                    throw std::runtime_error("internal bounds error");
                }

                const int oldValue = fp.get();
                auto clone = fp.clone(oldValue);
                clone.setFloat(value);
                if (clone.get() == oldValue) {
                    os << "No change: " << help << " is already " << fp.getFloat()
                       << AnsiOstream::endl;
                    return;
                }

                clone.set(oldValue);
                fp.setFloat(value);
                os << "Changed " << help << " from " << clone.getFloat() << " to " << fp.getFloat()
                   << AnsiOstream::endl;
                this->graphicsSettingsChanged();
            },
            "set " + help);
    };

    using namespace syntax;

    const auto argBool = TokenMatcher::alloc<ArgBool>();
    const auto argInt = TokenMatcher::alloc<ArgInt>();
    const auto optArgEquals = TokenMatcher::alloc<ArgOptionalChar>(char_consts::C_EQUALS);

    auto makeFixedPointArg = [optArgEquals,
                              &makeSetFixedPoint](FixedPoint<1> &fp,
                                                  const std::string &help) -> SharedConstSublist {
        const auto min = fp.clone(fp.min).getFloat();
        const auto max = fp.clone(fp.max).getFloat();
        return syn(help,
                   optArgEquals,
                   TokenMatcher::alloc_copy<ArgFloat>(ArgFloat::withMinMax(min, max)),
                   makeSetFixedPoint(fp, help));
    };

    auto &advanced = setConfig().canvas.advanced;

    // static because it has no captures
    static const auto getZoom = []() -> float {
        if (auto primary = MapCanvas::getPrimary()) {
            return primary->getRawZoom();
        }
        return 1.f;
    };

    const auto setZoom = [this](float f) -> bool {
        if (auto primary = MapCanvas::getPrimary()) {
            primary->setZoom(f);
            this->graphicsSettingsChanged();
            return true;
        }

        return false;
    };

    const auto zoomSyntax = std::invoke([&setZoom]() -> SharedConstSublist {
        const auto argZoom = TokenMatcher::alloc_copy<ArgFloat>(
            ArgFloat::withMinMax(ScaleFactor::MIN_VALUE, ScaleFactor::MAX_VALUE));
        const auto acceptZoom = Accept(
            [&setZoom](User &user, const Pair *const args) -> void {
                auto &os = user.getOstream();

                if (args == nullptr || !args->car.isFloat()) {
                    throw std::runtime_error("internal type error");
                }

                const float value = args->car.getFloat();
                const auto min = ScaleFactor::MIN_VALUE;
                const auto max = ScaleFactor::MAX_VALUE;
                if (value < min || value > max) {
                    throw std::runtime_error("internal bounds error");
                }

                const float oldValue = getZoom();
                if (utils::equals(value, oldValue)) {
                    os << "No change: zoom is already " << oldValue << AnsiOstream::endl;
                    return;
                }

                if (setZoom(value)) {
                    os << "Changed zoom from " << oldValue << " to " << value << AnsiOstream::endl;
                } else {
                    os << "Unable to change zoom.\n";
                }
            },
            "set zoom");
        return syn("zoom", syn("set", argZoom, acceptZoom));
    });

    const auto opt = [this, argBool, optArgEquals](const char *const name,
                                                   NamedConfig<bool> &conf,
                                                   std::string help) {
        return syn(name,
                   optArgEquals,
                   argBool,
                   Accept(
                       [this, &conf](User &user, const Pair *const args) {
                           const auto value = deref(args).car.getBool();
                           auto &os = user.getOstream();

                           if (conf.get() == value) {
                               os << conf.getName() << " is already " << BoolAlpha(value)
                                  << AnsiOstream::endl;
                               return;
                           }

                           conf.set(value);
                           os << "Set " << conf.getName() << " = " << BoolAlpha(value)
                              << AnsiOstream::endl;
                           graphicsSettingsChanged();
                       },
                       std::move(help)));
    };

    const auto configSyntax = syn(
        syn("mode",
            syn("play",
                Accept(
                    [this](User &user, auto) {
                        setMode(MapModeEnum::PLAY);
                        send_ok(user.getOstream());
                    },
                    "play mode")),
            syn("mapping",
                Accept(
                    [this](User &user, auto) {
                        setMode(MapModeEnum::MAP);
                        send_ok(user.getOstream());
                    },
                    "mapping mode")),
            syn("emulation",
                Accept(
                    [this](User &user, auto) {
                        setMode(MapModeEnum::OFFLINE);
                        send_ok(user.getOstream());
                    },
                    "offline emulation mode"))),
        syn("file",
            // TODO: add a command to show what's different from the factory default values,
            // and another command to show what's different from the current save file,
            // or just a list of {key, default, saved, current}?
            syn("save",
                Accept(
                    [](User &user, auto) {
                        auto &os = user.getOstream();
                        os << "Saving config file...\n";
                        getConfig().write();
                        os << "Saved.\n";
                    },
                    "save config file")),
            syn("load",
                Accept(
                    [this](User &user, auto) {
                        auto &os = user.getOstream();
                        if (isConnected()) {
                            os << "You must disconnect before you can reload the saved configuration.\n";
                            return;
                        }
                        os << "Loading saved file...\n";
                        setConfig().read();
                        send_ok(os);
                    },
                    "read config file")),
            syn("edit",
                Accept(
                    [this](User &user, auto) {
                        auto &os = user.getOstream();
                        if (isConnected()) {
                            os << "You must disconnect before you can edit the saved configuration.\n";
                            return;
                        }
                        os << "Opening configuration editor...\n";

                        QString content;
                        QString fileName;
                        {
                            QTemporaryFile temp(QDir::tempPath() + "/mmapper_XXXXXX.ini");
                            temp.setAutoRemove(false);
                            if (!temp.open()) {
                                os << "Failed to create temporary file.\n";
                                return;
                            }
                            fileName = temp.fileName();
                            temp.close();

                            {
                                QSettings settings(fileName, QSettings::IniFormat);
                                getConfig().writeTo(settings);
                                settings.sync();
                            }

                            QFile file(fileName);
                            if (file.open(QIODevice::ReadOnly)) {
                                content = QString::fromUtf8(file.readAll());
                                file.close();
                            }
                            QFile::remove(fileName);
                        }

                        if (content.isEmpty()) {
                            os << "Configuration is empty or failed to export.\n";
                            return;
                        }

                        // REVISIT: Ideally we support external editor as well
                        auto *editor = new RemoteEditWidget(true,
                                                            "MMapper Client Configuration",
                                                            content,
                                                            nullptr);
                        QObject::connect(editor,
                                         &RemoteEditWidget::sig_save,
                                         [weakParser = QPointer<AbstractParser>(this)](
                                             const QString &edited) {
                                             if (weakParser.isNull()) {
                                                 return;
                                             }

                                             QTemporaryFile tempRead(QDir::tempPath()
                                                                     + "/mmapper_XXXXXX.ini");
                                             tempRead.setAutoRemove(true);
                                             if (tempRead.open()) {
                                                 QString name = tempRead.fileName();
                                                 tempRead.write(edited.toUtf8());
                                                 tempRead.close();

                                                 {
                                                     auto &cfg = setConfig();
                                                     QSettings settings(name, QSettings::IniFormat);
                                                     cfg.readFrom(settings);
                                                     cfg.write();
                                                 }

                                                 weakParser->sendToUser(
                                                     SendToUserSourceEnum::FromMMapper,
                                                     "\nConfiguration imported and persisted.\n");
                                                 weakParser->sendOkToUser();
                                             }
                                         });

                        editor->setAttribute(Qt::WA_DeleteOnClose);
                        editor->show();
                        editor->activateWindow();
                    },
                    "edit client configuration")),
            syn("factory",
                "reset",
                TokenMatcher::alloc<ArgStringExact>("Yes, I'm sure!"),
                Accept(
                    [this](User &user, auto) {
                        auto &os = user.getOstream();
                        if (isConnected()) {
                            os << "You must disconnect before you can do a factory reset.\n";
                            return;
                        }
                        // REVISIT: only allow this when you're disconnected?
                        os << "Performing factory reset...\n";
                        setConfig().reset();
                        os << "WARNING: You have just reset your configuration.\n";
                    },
                    "factory reset the config"))),
        syn("map",
            syn("colors",
                syn(syn("list", listColors),
                    syn("set",
                        TokenMatcher::alloc<ArgNamedColor>(),
                        optArgEquals,
                        TokenMatcher::alloc<ArgHexColor>(),
                        setNamedColor))),
            syn("perf-stats",
                syn("set", opt("enabled", advanced.printPerfStats, "enable/disable stats"))),
            zoomSyntax,
            syn("3d-camera",
                syn("set",
                    opt("enabled", advanced.use3D, "enable/disable 3d camera"),
                    opt("auto-tilt", advanced.autoTilt, "enable/disable 3d auto tilt"),
                    makeFixedPointArg(advanced.fov, "fov"),
                    makeFixedPointArg(advanced.verticalAngle, "pitch"),
                    makeFixedPointArg(advanced.horizontalAngle, "yaw"),
                    makeFixedPointArg(advanced.layerHeight, "layer-height")))));

    eval("config", configSyntax, cmd);
}
