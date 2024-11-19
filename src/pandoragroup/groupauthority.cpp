// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "groupauthority.h"

#include "../configuration/configuration.h"
#include "../global/logging.h"
#include "../global/utils.h"
#include "GroupSocket.h"
#include "enums.h"

#include <exception>
#include <iostream>
#include <memory>

#include <QDebug>

#ifndef MMAPPER_NO_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#if OPENSSL_VERSION_NUMBER > 0x30000000L /* 3.0.x */
#include <openssl/encoder.h>
#endif

using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using X509_ptr = std::unique_ptr<X509, decltype(&::X509_free)>;
using BIO_MEM_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

/* Generates a 2048-bit RSA key. */
NODISCARD static EVP_PKEY_ptr generatePrivateKey()
{
    /* Allocate memory for the EVP_PKEY and BIGNUM structures. */
    EVP_PKEY_ptr pkey(EVP_PKEY_new(), ::EVP_PKEY_free);
    if (!pkey)
        throw std::runtime_error("Unable to create EVP_PKEY structure.");

    using BIGNUM_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
    BIGNUM_ptr bn(BN_new(), ::BN_free);
    if (!bn)
        throw std::runtime_error("Unable to create BIGNUM structure.");
    BN_set_word(bn.get(), RSA_F4);

    using EVP_PKEY_CTX_ptr = std::unique_ptr<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>;
    EVP_PKEY_CTX_ptr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), ::EVP_PKEY_CTX_free);
    if (!ctx)
        throw std::runtime_error("Unable to allocate public key algorithm contex.");

    if (EVP_PKEY_keygen_init(ctx.get()) <= 0)
        throw std::runtime_error("Unable to initialize a public key algorithm.");

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 2048) <= 0)
        throw std::runtime_error("Unable to generate 2048-bit RSA key.");

    auto key = pkey.get();
    if (EVP_PKEY_keygen(ctx.get(), &key) <= 0)
        throw std::runtime_error("Unable to write generated key to private key.");

    /* The key has been generated, return it. */
    return pkey;
}

/* Generates a self-signed x509 certificate. */
NODISCARD static X509_ptr generateX509(const EVP_PKEY_ptr &pkey)
{
    /* Allocate memory for the X509 structure. */
    X509_ptr x509(X509_new(), ::X509_free);
    if (!x509)
        throw std::runtime_error("Unable to create X509 structure.");

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
                               as_unsigned_cstring(GROUP_ORGANIZATION),
                               -1,
                               -1,
                               0);
    X509_NAME_add_entry_by_txt(name,
                               "OU",
                               MBSTRING_ASC,
                               as_unsigned_cstring(GROUP_ORGANIZATIONAL_UNIT),
                               -1,
                               -1,
                               0);
    X509_NAME_add_entry_by_txt(name,
                               "CN",
                               MBSTRING_ASC,
                               as_unsigned_cstring(GROUP_COMMON_NAME),
                               -1,
                               -1,
                               0);

    /* Now set the issuer name. */
    X509_set_issuer_name(x509.get(), name);

    /* Actually sign the certificate with our key. */
    if (!X509_sign(x509.get(), pkey.get(), EVP_sha1()))
        throw std::runtime_error("Error signing certificate.");

    return x509;
}

NODISCARD static QSslCertificate toSslCertificate(const X509_ptr &x509)
{
    BIO_MEM_ptr bio(BIO_new(BIO_s_mem()), ::BIO_free);

    int rc = PEM_write_bio_X509(bio.get(), x509.get());
    unsigned long err = ERR_get_error();

    if (rc != 1) {
        MMLOG_ERROR() << "PEM_write_bio_X509 failed, error " << err << ", " << std::hex << "0x"
                      << err;
        throw std::runtime_error("Encoding certificate failed.");
    }

    BUF_MEM *mem = nullptr;
    BIO_get_mem_ptr(bio.get(), &mem);
    err = ERR_get_error();

    if (!mem || !mem->data || !mem->length) {
        MMLOG_ERROR() << "BIO_get_mem_ptr failed, error " << err << ", " << std::hex << "0x" << err;
        throw std::runtime_error("Fetching certificate failed.");
    }

    QByteArray ba(mem->data, static_cast<int>(mem->length));
    setConfig().groupManager.certificate = ba;
    return QSslCertificate(ba, QSsl::EncodingFormat::Pem);
}

NODISCARD static QSslKey toSslKey(const EVP_PKEY_ptr &pkey)
{
    BIO_MEM_ptr bio(BIO_new(BIO_s_mem()), ::BIO_free);
    unsigned long err = ERR_get_error();

#if OPENSSL_VERSION_NUMBER > 0x30000000L /* 3.0.x */
    if (EVP_PKEY_get_base_id(pkey.get()) != EVP_PKEY_RSA)
        throw std::runtime_error("Public key of x509 is not of type RSA.");

    using OSSL_ENCODER_CTX_ptr
        = std::unique_ptr<OSSL_ENCODER_CTX, decltype(&::OSSL_ENCODER_CTX_free)>;

    static constexpr const auto selection = OSSL_KEYMGMT_SELECT_PRIVATE_KEY;
    static constexpr const char *const format = "PEM";
    static constexpr const char *const structure = "rsa";
    OSSL_ENCODER_CTX_ptr
        ectx(OSSL_ENCODER_CTX_new_for_pkey(pkey.get(), selection, format, structure, nullptr),
             ::OSSL_ENCODER_CTX_free);
    if (!ectx)
        throw std::runtime_error("No suitable potential encoders found,");

    if (!OSSL_ENCODER_to_bio(ectx.get(), bio.get()))
        throw std::runtime_error("Encoding PEM failed.");
#else
    RSA *const rsa = EVP_PKEY_get1_RSA(pkey.get()); // Get the underlying RSA key
    int rc = PEM_write_bio_RSAPrivateKey(bio.get(), rsa, nullptr, nullptr, 0, nullptr, nullptr);

    if (rc != 1) {
        MMLOG_ERROR() << "PEM_write_bio_RSAPrivateKey failed, error " << err << ", " << std::hex
                      << "0x" << err;
        throw std::runtime_error("");
    }
#endif

    BUF_MEM *mem = nullptr;
    BIO_get_mem_ptr(bio.get(), &mem);
    err = ERR_get_error();

    if (!mem || !mem->data || !mem->length) {
        MMLOG_ERROR() << "BIO_get_mem_ptr failed, error " << err << ", " << std::hex << "0x" << err;
        throw std::runtime_error("Fetching encoded key failed.");
    }

    QByteArray ba(mem->data, static_cast<int>(mem->length));
    setConfig().groupManager.privateKey = ba;
    return QSslKey(ba, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
}

void GroupAuthority::refresh()
{
    // https://gist.github.com/nathan-osman/5041136
    // https://forum.qt.io/topic/45728/generating-cert-key-during-run-time-for-qsslsocket/7
    try {
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
        emit sig_secretRefreshed(getSecret());
    } catch (const std::exception &ex) {
        qWarning() << "Refresh error because:" << ex.what();
    }
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

NODISCARD static inline QString getMetadataKey(const GroupSecret &secret,
                                               const GroupMetadataEnum meta)
{
    const auto get_prefix = [&meta]() {
        switch (meta) {
        case GroupMetadataEnum::CERTIFICATE:
            return "certificate";
        case GroupMetadataEnum::NAME:
            return "name";
        case GroupMetadataEnum::IP_ADDRESS:
            return "ip";
        case GroupMetadataEnum::LAST_LOGIN:
            return "last_login";
        case GroupMetadataEnum::PORT:
            return "port";
        default:
            abort();
        }
    };
    return QString("%1-%2").arg(get_prefix(), secret.toLower().constData());
}

GroupAuthority::GroupAuthority(QObject *const parent)
    : QObject(parent)
{
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

    emit sig_secretRevoked(secret);

    // Remove from model
    for (int i = 0; i <= model.rowCount(); i++) {
        QModelIndex index = model.index(i);
        const QString targetSecret = model.data(index, Qt::DisplayRole).toString();
        if (targetSecret.compare(secret, Qt::CaseInsensitive) == 0) {
            model.removeRow(i);

            // Update configuration
            auto &conf = setConfig().groupManager;
            conf.authorizedSecrets = model.stringList();

            for (const GroupMetadataEnum type : ALL_GROUP_METADATA) {
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

bool GroupAuthority::validCertificate(const GroupSocket &connection)
{
    const GroupSecret &targetSecret = connection.getSecret();
    const QString &storedCertificate = getMetadata(targetSecret, GroupMetadataEnum::CERTIFICATE);
    if (storedCertificate.isEmpty())
        return true;

    const QString targetCertficiate = connection.getPeerCertificate().toPem();
    const bool certificatesMatch = targetCertficiate.compare(storedCertificate, Qt::CaseInsensitive)
                                   == 0;
    return certificatesMatch;
}

QString GroupAuthority::getMetadata(const GroupSecret &secret, const GroupMetadataEnum meta)
{
    const auto &metadata = getConfig().groupManager.secretMetadata;
    return metadata[getMetadataKey(secret, meta)].toString();
}

void GroupAuthority::setMetadata(const GroupSecret &secret,
                                 const GroupMetadataEnum meta,
                                 const QString &value)
{
    auto &metadata = setConfig().groupManager.secretMetadata;
    metadata[getMetadataKey(secret, meta)] = value;
}
