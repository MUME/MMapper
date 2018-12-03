/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "groupauthority.h"

#include <exception>
#include <iostream>
#include <memory>
#include <QDebug>

#include "../configuration/configuration.h"
#include "../global/enums.h"
#include "GroupSocket.h"

#ifndef MMAPPER_NO_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using BIGNUM_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
using X509_ptr = std::unique_ptr<X509, decltype(&::X509_free)>;
using BIO_MEM_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

/* Generates a 2048-bit RSA key. */
static EVP_PKEY_ptr generatePrivateKey()
{
    /* Allocate memory for the EVP_PKEY and BIGNUM structures. */
    EVP_PKEY_ptr pkey(EVP_PKEY_new(), ::EVP_PKEY_free);
    if (!pkey) {
        std::cerr << "Unable to create EVP_PKEY structure." << std::endl;
        throw std::runtime_error("");
    }

    BIGNUM_ptr bn(BN_new(), ::BN_free);
    if (!bn) {
        std::cerr << "Unable to create BIGNUM structure." << std::endl;
        throw std::runtime_error("");
    }
    BN_set_word(bn.get(), RSA_F4);

    /* Generate the RSA key and assign it to pkey. */
    RSA *rsa = RSA_new(); // RSA object is freed with EVP_PKEY_free
    if (RSA_generate_key_ex(rsa, 2048, bn.get(), nullptr) != 1) {
        std::cerr << "Unable to generate 2048-bit RSA key." << std::endl;
        throw std::runtime_error("");
    }

    if (!EVP_PKEY_assign_RSA(pkey.get(), rsa)) {
        std::cerr << "Unable to assign 2048-bit RSA to private key." << std::endl;
        throw std::runtime_error("");
    }

    /* The key has been generated, return it. */
    return pkey;
}

/* Generates a self-signed x509 certificate. */
static X509_ptr generateX509(const EVP_PKEY_ptr &pkey)
{
    /* Allocate memory for the X509 structure. */
    X509_ptr x509(X509_new(), ::X509_free);
    if (!x509) {
        std::cerr << "Unable to create X509 structure." << std::endl;
        throw std::runtime_error("");
    }

    /* Set the serial number. */
    ASN1_INTEGER_set(X509_get_serialNumber(x509.get()), 1);

    /* This certificate is valid from now until exactly ten years from now. */
    X509_gmtime_adj(X509_get_notBefore(x509.get()), 0);
    X509_gmtime_adj(X509_get_notAfter(x509.get()), 315360000L);

    /* Set the public key for our certificate. */
    X509_set_pubkey(x509.get(), pkey.get());

    /* We want to copy the subject name to the issuer name. */
    X509_NAME *name = X509_get_subject_name(x509.get());

    /* Set the X509 names. */
    X509_NAME_add_entry_by_txt(name,
                               "O",
                               MBSTRING_ASC,
                               reinterpret_cast<const unsigned char *>(GROUP_ORGANIZATION),
                               -1,
                               -1,
                               0);
    X509_NAME_add_entry_by_txt(name,
                               "OU",
                               MBSTRING_ASC,
                               reinterpret_cast<const unsigned char *>(GROUP_ORGANIZATIONAL_UNIT),
                               -1,
                               -1,
                               0);
    X509_NAME_add_entry_by_txt(name,
                               "CN",
                               MBSTRING_ASC,
                               reinterpret_cast<const unsigned char *>(GROUP_COMMON_NAME),
                               -1,
                               -1,
                               0);

    /* Now set the issuer name. */
    X509_set_issuer_name(x509.get(), name);

    /* Actually sign the certificate with our key. */
    if (!X509_sign(x509.get(), pkey.get(), EVP_sha1())) {
        std::cerr << "Error signing certificate." << std::endl;
        throw std::runtime_error("");
    }

    return x509;
}

static QSslCertificate toSslCertificate(const X509_ptr &x509)
{
    BIO_MEM_ptr bio(BIO_new(BIO_s_mem()), ::BIO_free);

    int rc = PEM_write_bio_X509(bio.get(), x509.get());
    unsigned long err = ERR_get_error();

    if (rc != 1) {
        std::cerr << "PEM_write_bio_X509 failed, error " << err << ", ";
        std::cerr << std::hex << "0x" << err;
        throw std::runtime_error("");
    }

    BUF_MEM *mem = nullptr;
    BIO_get_mem_ptr(bio.get(), &mem);
    err = ERR_get_error();

    if (!mem || !mem->data || !mem->length) {
        std::cerr << "BIO_get_mem_ptr failed, error " << err << ", ";
        std::cerr << std::hex << "0x" << err;
        throw std::runtime_error("");
    }

    QByteArray ba(mem->data, static_cast<int>(mem->length));
    setConfig().groupManager.certificate = ba;
    return QSslCertificate(ba);
}

static QSslKey toSslKey(const EVP_PKEY_ptr &pkey)
{
    BIO_MEM_ptr bio(BIO_new(BIO_s_mem()), ::BIO_free);

    RSA *const rsa = EVP_PKEY_get1_RSA(pkey.get()); // Get the underlying RSA key
    int rc = PEM_write_bio_RSAPrivateKey(bio.get(), rsa, nullptr, nullptr, 0, nullptr, nullptr);
    unsigned long err = ERR_get_error();

    if (rc != 1) {
        std::cerr << "PEM_write_bio_RSAPrivateKey failed, error " << err << ", ";
        std::cerr << std::hex << "0x" << err;
        throw std::runtime_error("");
    }

    BUF_MEM *mem = nullptr;
    BIO_get_mem_ptr(bio.get(), &mem);
    err = ERR_get_error();

    if (!mem || !mem->data || !mem->length) {
        std::cerr << "BIO_get_mem_ptr failed, error " << err << ", ";
        std::cerr << std::hex << "0x" << err;
        throw std::runtime_error("");
    }

    QByteArray ba(mem->data, static_cast<int>(mem->length));
    setConfig().groupManager.privateKey = ba;
    return QSslKey(ba, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
}

void GroupAuthority::refresh()
{
    // https://gist.github.com/nathan-osman/5041136
    // https://forum.qt.io/topic/45728/generating-cert-key-during-run-time-for-qsslsocket/7
    const auto pkey = generatePrivateKey();
    const auto x509 = generateX509(pkey);
    certificate = toSslCertificate(x509);
    if (certificate.isNull()) {
        qWarning() << "Unable to generate a valid certificate" << certificate;
    }
    key = toSslKey(pkey);
    if (key.isNull()) {
        qWarning() << "Unable to generate a valid private key" << key;
    }
    emit secretRefreshed(getSecret());
}
#else
void GroupAuthority::refresh()
{
    certificate.clear();
    setConfig().groupManager.certificate = "";
    key.clear();
    setConfig().groupManager.privateKey = "";
}
#endif

namespace enums {

const std::array<GroupMetadata, NUM_GROUP_METADATA> &getAllGroupMetadata()
{
    static const auto g_all_types = genEnumValues<GroupMetadata, NUM_GROUP_METADATA>();
    return g_all_types;
}

} // namespace enums

static inline QString getMetadataKey(const GroupSecret &secret, const GroupMetadata meta)
{
    const auto get_prefix = [](auto meta) {
        switch (meta) {
        case GroupMetadata::CERTIFICATE:
            return "certificate";
        case GroupMetadata::NAME:
            return "name";
        case GroupMetadata::IP_ADDRESS:
            return "ip";
        case GroupMetadata::LAST_LOGIN:
            return "last_login";
        default:
            abort();
        }
    };
    return QString("%1-%2").arg(get_prefix(meta)).arg(secret.toLower().constData());
}

GroupAuthority::GroupAuthority(QObject *const parent)
    : QObject(parent)
{
    qRegisterMetaType<GroupSecret>("GroupSecret");

    // Always utilize a temporary keychain
    qputenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", "1");

    const auto &groupManager = getConfig().groupManager;
    if (groupManager.certificate.isEmpty() || groupManager.privateKey.isEmpty()) {
        // First time use workflow. Generate a certificate.
        refresh();
    } else {
        // Load the certifcate and private key from the config
        certificate = QSslCertificate(groupManager.certificate);
        if (certificate.isNull()) {
            qWarning() << "Unable to load a valid certificate" << certificate;
        }
        key = QSslKey(groupManager.privateKey, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        if (key.isNull()) {
            qWarning() << "Unable to load a valid private key" << key;
        }
        if (certificate.isNull() || key.isNull()) {
            qWarning() << "Refreshing invalid certificate and private key:" << certificate << key;
            refresh();
        }
    }

    if (QDateTime::currentDateTime() >= certificate.expiryDate()) {
        qWarning() << "Refreshing certificate which expired on:" << certificate.expiryDate();
        refresh();
    }

    // Prime model
    model.setStringList(groupManager.authorizedSecrets);
}

GroupSecret GroupAuthority::getSecret() const
{
    // SHA-1 isn't very secure but at 40 characters it fits within a line for tells
    return certificate.digest(QCryptographicHash::Algorithm::Sha1).toHex();
}

bool GroupAuthority::add(const GroupSecret &secret)
{
    if (validSecret(secret))
        return false;

    // Update model
    if (model.insertRow(model.rowCount())) {
        QModelIndex index = model.index(model.rowCount() - 1, 0);
        model.setData(index, secret.toLower(), Qt::DisplayRole);

        // Update configuration
        setConfig().groupManager.authorizedSecrets = model.stringList();
        return true;
    }

    return false;
}

bool GroupAuthority::remove(const GroupSecret &secret)
{
    if (!validSecret(secret))
        return false;

    emit secretRevoked(secret);

    // Remove from model
    for (int i = 0; i <= model.rowCount(); i++) {
        QModelIndex index = model.index(i);
        const QString targetSecret = model.data(index, Qt::DisplayRole).toString();
        if (targetSecret.compare(secret, Qt::CaseInsensitive) == 0) {
            model.removeRow(i);

            // Update configuration
            auto &conf = setConfig().groupManager;
            conf.authorizedSecrets = model.stringList();

            for (auto type : ALL_GROUP_METADATA) {
                conf.secretMetadata.remove(getMetadataKey(secret, type));
            }
            return true;
        }
    }
    return false;
}

bool GroupAuthority::validSecret(const GroupSecret &secret) const
{
    return model.stringList().contains(secret.toLower());
}

bool GroupAuthority::validCertificate(const GroupSocket *connection) const
{
    const GroupSecret &targetSecret = connection->getSecret();
    const QString &storedCertificate = getMetadata(targetSecret, GroupMetadata::CERTIFICATE);
    if (storedCertificate.isEmpty())
        return true;

    const QString targetCertficiate = connection->getPeerCertificate().toPem();
    const bool certificatesMatch = targetCertficiate.compare(storedCertificate, Qt::CaseInsensitive)
                                   == 0;
    return certificatesMatch;
}

QString GroupAuthority::getMetadata(const GroupSecret &secret, const GroupMetadata meta) const
{
    const auto &metadata = getConfig().groupManager.secretMetadata;
    return metadata[getMetadataKey(secret, meta)].toString();
}

void GroupAuthority::setMetadata(const GroupSecret &secret,
                                 const GroupMetadata meta,
                                 const QString &value)
{
    auto &metadata = setConfig().groupManager.secretMetadata;
    metadata[getMetadataKey(secret, meta)] = value;
}
