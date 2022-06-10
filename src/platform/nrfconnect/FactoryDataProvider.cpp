/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "FactoryDataProvider.h"
#include <crypto/CHIPCryptoPAL.h>

#include <logging/log.h>

namespace chip {
namespace {
CHIP_ERROR LoadKeypairFromRaw(ByteSpan privateKey, ByteSpan publicKey, Crypto::P256Keypair & keypair)
{
    Crypto::P256SerializedKeypair serializedKeypair;
    ReturnErrorOnFailure(serializedKeypair.SetLength(privateKey.size() + publicKey.size()));
    memcpy(serializedKeypair.Bytes(), publicKey.data(), publicKey.size());
    memcpy(serializedKeypair.Bytes() + publicKey.size(), privateKey.data(), privateKey.size());
    return keypair.Deserialize(serializedKeypair);
}
} // namespace

namespace DeviceLayer {

void FactoryDataProvider::LoadFactoryData()
{
    if (!GetFactoryData(mFactoryDataBuffer, kFactoryDataPartitionSize, kFactoryDataPartitionAddress, &mFactoryData))
    {
        ChipLogError(DeviceLayer, "Failed to load factory data");
        return;
    }
}

CHIP_ERROR FactoryDataProvider::GetCertificationDeclaration(MutableByteSpan & outBuffer)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}

CHIP_ERROR FactoryDataProvider::GetFirmwareInformation(MutableByteSpan & out_firmware_info_buffer)
{
    ReturnErrorCodeIf(out_firmware_info_buffer.size() < mFactoryData.fw_info.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData.fw_info.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    memcpy(out_firmware_info_buffer.data(), mFactoryData.fw_info.data, mFactoryData.fw_info.len);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetDeviceAttestationCert(MutableByteSpan & outBuffer)
{
    ReturnErrorCodeIf(outBuffer.size() < mFactoryData.dac_cert.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData.dac_cert.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    memcpy(outBuffer.data(), mFactoryData.dac_cert.data, mFactoryData.dac_cert.len);

    outBuffer.reduce_size(mFactoryData.dac_cert.len);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetProductAttestationIntermediateCert(MutableByteSpan & outBuffer)
{
    ReturnErrorCodeIf(outBuffer.size() < mFactoryData.pai_cert.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData.pai_cert.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    memcpy(outBuffer.data(), mFactoryData.pai_cert.data, mFactoryData.pai_cert.len);

    outBuffer.reduce_size(mFactoryData.pai_cert.len);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::SignWithDeviceAttestationKey(const ByteSpan & digestToSign, MutableByteSpan & outSignBuffer)
{
    Crypto::P256ECDSASignature signature;
    Crypto::P256Keypair keypair;

    VerifyOrReturnError(IsSpanUsable(outSignBuffer), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(IsSpanUsable(digestToSign), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(outSignBuffer.size() >= signature.Capacity(), CHIP_ERROR_BUFFER_TOO_SMALL);

    // Extract public key from DAC cert.
    MutableByteSpan dacCertSpan{ reinterpret_cast<uint8_t *>(mFactoryData.dac_cert.data), mFactoryData.dac_cert.len };
    chip::Crypto::P256PublicKey dacPublicKey;
    CHIP_ERROR err = chip::Crypto::ExtractPubkeyFromX509Cert(dacCertSpan, dacPublicKey);

    // Read private key from factory data.
    uint8_t privKeyBuf[kDACPrivateKeyLength];
    size_t privKeyLen = sizeof(privKeyBuf);

    memcpy(privKeyBuf, mFactoryData.dac_priv_key.data, mFactoryData.dac_priv_key.len);

    ReturnErrorOnFailure(
        LoadKeypairFromRaw(ByteSpan(privKeyBuf, privKeyLen), ByteSpan(dacPublicKey.Bytes(), dacPublicKey.Length()), keypair));
    ReturnErrorOnFailure(keypair.ECDSA_sign_hash(digestToSign.data(), digestToSign.size(), signature));

    return CopySpanToMutableSpan(ByteSpan{ signature.ConstBytes(), signature.Length() }, outSignBuffer);
}

CHIP_ERROR FactoryDataProvider::GetSetupDiscriminator(uint16_t & setupDiscriminator)
{
    setupDiscriminator = mFactoryData.discriminator;
    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::SetSetupDiscriminator(uint16_t setupDiscriminator)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}

CHIP_ERROR FactoryDataProvider::GetSpake2pIterationCount(uint32_t & iterationCount)
{
    ReturnErrorCodeIf(mFactoryData.spake2_it == 0, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    iterationCount = mFactoryData.spake2_it;
    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetSpake2pSalt(MutableByteSpan & saltBuf)
{
    ReturnErrorCodeIf(saltBuf.size() < mFactoryData.spake2_salt.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData.spake2_salt.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    memcpy(saltBuf.data(), mFactoryData.spake2_salt.data, mFactoryData.spake2_salt.len);

    saltBuf.reduce_size(mFactoryData.spake2_salt.len);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetSpake2pVerifier(MutableByteSpan & verifierBuf, size_t & verifierLen)
{
    ReturnErrorCodeIf(verifierBuf.size() < mFactoryData.spake2_verifier.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData.spake2_verifier.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    memcpy(verifierBuf.data(), mFactoryData.spake2_verifier.data, mFactoryData.spake2_verifier.len);

    verifierLen = mFactoryData.spake2_verifier.len;

    verifierBuf.reduce_size(verifierLen);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetSetupPasscode(uint32_t & setupPasscode)
{
    ReturnErrorCodeIf(mFactoryData.passcode == 0, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    setupPasscode = mFactoryData.passcode;
    return CHIP_ERROR_NOT_IMPLEMENTED;
}

CHIP_ERROR FactoryDataProvider::SetSetupPasscode(uint32_t setupPasscode)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}

CHIP_ERROR FactoryDataProvider::GetVendorName(char * buf, size_t bufSize)
{
    ReturnErrorCodeIf(bufSize < mFactoryData.vendor_name.len + 1, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData.vendor_name.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    memcpy(buf, mFactoryData.vendor_name.data, mFactoryData.vendor_name.len);
    memset(buf + mFactoryData.vendor_name.len, 0, 1);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetVendorId(uint16_t & vendorId)
{
    vendorId = mFactoryData.vendor_id;
    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetProductName(char * buf, size_t bufSize)
{
    ReturnErrorCodeIf(bufSize < mFactoryData.product_name.len + 1, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData.product_name.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    memcpy(buf, mFactoryData.product_name.data, mFactoryData.product_name.len);
    memset(buf + mFactoryData.product_name.len, 0, 1);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetProductId(uint16_t & productId)
{
    productId = mFactoryData.product_id;
    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetSerialNumber(char * buf, size_t bufSize)
{
    ReturnErrorCodeIf(bufSize < mFactoryData.sn.len + 1, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData.sn.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    memcpy(buf, mFactoryData.sn.data, mFactoryData.sn.len);
    memset(buf + mFactoryData.sn.len, 0, 1);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & day)
{
    year  = mFactoryData.date_year;
    month = mFactoryData.date_month;
    day   = mFactoryData.date_day;
    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetHardwareVersion(uint16_t & hardwareVersion)
{
    hardwareVersion = mFactoryData.hw_ver;
    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetHardwareVersionString(char * buf, size_t bufSize)
{
    ReturnErrorCodeIf(bufSize < mFactoryData.hw_ver_str.len + 1, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData.hw_ver_str.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    memcpy(buf, mFactoryData.hw_ver_str.data, mFactoryData.hw_ver_str.len);
    memset(buf + mFactoryData.hw_ver_str.len, 0, 1);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProvider::GetRotatingDeviceIdUniqueId(MutableByteSpan & uniqueIdSpan)
{
    ReturnErrorCodeIf(uniqueIdSpan.size() < mFactoryData.rd_uid.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData.rd_uid.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    memcpy(uniqueIdSpan.data(), mFactoryData.rd_uid.data, mFactoryData.rd_uid.len);

    return CHIP_NO_ERROR;
}

} // namespace DeviceLayer
} // namespace chip
