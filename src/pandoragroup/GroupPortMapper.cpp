// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "GroupPortMapper.h"

#include <memory>
#include <QDebug>

#ifndef MMAPPER_NO_MINIUPNPC
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#endif

struct NODISCARD GroupPortMapper::Pimpl
{
    Pimpl() = default;
    virtual ~Pimpl();

private:
    NODISCARD virtual QByteArray virt_tryGetExternalIp() = 0;
    NODISCARD virtual bool virt_tryAddPortMapping(quint16 port) = 0;
    NODISCARD virtual bool virt_tryDeletePortMapping(quint16 port) = 0;

public:
    NODISCARD QByteArray tryGetExternalIp() { return virt_tryGetExternalIp(); }
    NODISCARD bool tryAddPortMapping(quint16 port) { return virt_tryAddPortMapping(port); }
    NODISCARD bool tryDeletePortMapping(quint16 port) { return virt_tryDeletePortMapping(port); }
};

GroupPortMapper::Pimpl::~Pimpl() = default;

class NoopPortMapper final : public GroupPortMapper::Pimpl
{
public:
    NoopPortMapper() = default;
    ~NoopPortMapper() final;

private:
    NODISCARD QByteArray virt_tryGetExternalIp() final
    {
        // TODO: Use a 3rd party service like checkip.dyndns.org
        return "";
    }
    NODISCARD bool virt_tryAddPortMapping(const quint16) final { return false; }
    NODISCARD bool virt_tryDeletePortMapping(const quint16) final { return false; }
};

NoopPortMapper::~NoopPortMapper() = default;

#ifndef MMAPPER_NO_MINIUPNPC
static constexpr const auto UPNP_DESCRIPTION = "MMapper";
static constexpr const auto UPNP_WHITELISTED_PROTO = "TCP";
static constexpr const auto UPNP_PERMANENT_LEASE = "0";
using UPNPDev_ptr = std::unique_ptr<UPNPDev, decltype(&::freeUPNPDevlist)>;

class MiniUPnPcPortMapper final : public GroupPortMapper::Pimpl
{
private:
    UPNPDev_ptr deviceList{nullptr, ::freeUPNPDevlist};
    UPNPUrls urls;
    IGDdatas igdData;
    char lanAddress[64];
    int validIGDState = 0;

public:
    MiniUPnPcPortMapper()
    {
        int result = 0;
#if MINIUPNPC_API_VERSION < 14
        deviceList = UPNPDev_ptr(upnpDiscover(1000, nullptr, nullptr, 0, 0, &result),
                                 ::freeUPNPDevlist);
#else
        deviceList = UPNPDev_ptr(upnpDiscover(1000, nullptr, nullptr, 0, 0, 2, &result),
                                 ::freeUPNPDevlist);
#endif
        validIGDState = UPNP_GetValidIGD(deviceList.get(),
                                         &urls,
                                         &igdData,
                                         lanAddress,
                                         sizeof lanAddress);
        switch (validIGDState) {
        case 0:
            qInfo() << "No IGD found";
            break;
        case 1:
            qInfo() << "Valid IGD found";
            break;
        case 2:
            qInfo() << "Valid IGD has been found but it reported as not connected";
            break;
        case 3:
            qInfo() << "UPnP device has been found but was not recognized as an IGD";
            break;
        default:
            qWarning() << "UPNP_GetValidIGD returned an unknown result code" << validIGDState;
            break;
        }
    }
    ~MiniUPnPcPortMapper() final;

    NODISCARD bool validIGD() const { return validIGDState == 1; }

private:
    NODISCARD QByteArray virt_tryGetExternalIp() final
    {
        if (!validIGD())
            return "";

        // REVISIT: Expose the external IP in the preferences?
        static const constexpr int EXTERNAL_IP_ADDRESS_BYTES = 46; /* ipv6 requires 45 bytes */
        char externalAddress[EXTERNAL_IP_ADDRESS_BYTES];
        int result = UPNP_GetExternalIPAddress(urls.controlURL,
                                               igdData.first.servicetype,
                                               externalAddress);
        if (result != UPNPCOMMAND_SUCCESS) {
            qWarning() << "UPNP_GetExternalIPAddress returned" << result;
            return "";
        }

        if (!externalAddress[0]) {
            qWarning() << "IGD unable to retrieve external IP";
            return "";
        }

        qDebug() << "IGD reported external IP" << externalAddress;
        return QByteArray(externalAddress, EXTERNAL_IP_ADDRESS_BYTES);
    }

    NODISCARD bool virt_tryAddPortMapping(const quint16 port) final
    {
        if (!validIGD()) {
            qDebug() << "No IGD found to add a port mapping to";
            return false;
        }

        const auto portString = QString("%1").arg(port).toLocal8Bit();
        int result = UPNP_AddPortMapping(urls.controlURL,
                                         igdData.first.servicetype,
                                         portString.constData(),
                                         portString.constData(),
                                         lanAddress,
                                         UPNP_DESCRIPTION,
                                         UPNP_WHITELISTED_PROTO,
                                         nullptr,
                                         UPNP_PERMANENT_LEASE);
        if (result != UPNPCOMMAND_SUCCESS) {
            qWarning() << "UPNP_AddPortMapping failed with result code" << result;
            return false;
        }

        qDebug() << "Added IGD port mapping";
        return true;
    }

    NODISCARD bool virt_tryDeletePortMapping(const quint16 port) final
    {
        if (!validIGD()) {
            qDebug() << "No IGD found to remove a port mapping from";
            return false;
        }

        const auto portString = QString("%1").arg(port).toLocal8Bit();
        int result = UPNP_DeletePortMapping(urls.controlURL,
                                            igdData.first.servicetype,
                                            portString.constData(),
                                            UPNP_WHITELISTED_PROTO,
                                            nullptr);
        if (result != UPNPCOMMAND_SUCCESS) {
            qWarning() << "UPNP_DeletePortMapping failed with result code" << result;
            return false;
        }

        qDebug() << "Deleted IGD port mapping";
        return true;
    }
};

MiniUPnPcPortMapper::~MiniUPnPcPortMapper()
{
    if (validIGDState != 0)
        FreeUPNPUrls(&urls);
}

GroupPortMapper::GroupPortMapper()
    : m_pimpl{std::make_unique<MiniUPnPcPortMapper>()}
{}
#else
GroupPortMapper::GroupPortMapper()
    : m_pimpl{std::make_unique<NoopPortMapper>()}
{}
#endif

GroupPortMapper::~GroupPortMapper() = default;

QByteArray GroupPortMapper::tryGetExternalIp()
{
    return m_pimpl->tryGetExternalIp();
}

bool GroupPortMapper::tryAddPortMapping(const quint16 port)
{
    return m_pimpl->tryAddPortMapping(port);
}

bool GroupPortMapper::tryDeletePortMapping(const quint16 port)
{
    return m_pimpl->tryDeletePortMapping(port);
}
