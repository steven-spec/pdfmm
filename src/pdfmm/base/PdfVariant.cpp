/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfVariant.h"

#include "PdfLocale.h"
#include "PdfArray.h"
#include "PdfData.h"
#include "PdfDictionary.h"
#include "PdfOutputDevice.h"
#include "PdfParserObject.h"

#include <sstream>

using namespace mm;
using namespace std;

PdfVariant PdfVariant::NullValue;

PdfVariant::PdfVariant(PdfDataType type)
    : m_Data{ }, m_DataType(type) { }

PdfVariant::PdfVariant()
    : PdfVariant(PdfDataType::Null) { }

PdfVariant::PdfVariant(bool value)
    : PdfVariant(PdfDataType::Bool)
{
    m_Data.Bool = value;
}

PdfVariant::PdfVariant(int64_t value)
    : PdfVariant(PdfDataType::Number)
{
    m_Data.Number = value;
}

PdfVariant::PdfVariant(double value)
    : PdfVariant(PdfDataType::Real)
{
    m_Data.Real = value;
}

PdfVariant::PdfVariant(const PdfString& str)
    : PdfVariant(PdfDataType::String)
{
    m_Data.Data = new PdfString(str);
}

PdfVariant::PdfVariant(const PdfName& name)
    : PdfVariant(PdfDataType::Name)
{
    m_Data.Data = new PdfName(name);
}

PdfVariant::PdfVariant(const PdfReference& ref)
    : PdfVariant(PdfDataType::Reference)
{
    m_Data.Reference = ref;
}

PdfVariant::PdfVariant(const PdfArray& arr)
    : PdfVariant(PdfDataType::Array)
{
    m_Data.Data = new PdfArray(arr);
}

PdfVariant::PdfVariant(const PdfDictionary& dict)
    : PdfVariant(PdfDataType::Dictionary)
{
    m_Data.Data = new PdfDictionary(dict);
}

PdfVariant::PdfVariant(const PdfData& data)
    : PdfVariant(PdfDataType::RawData)
{
    m_Data.Data = new PdfData(data);
}

PdfVariant::PdfVariant(const PdfVariant& rhs)
    : PdfVariant(PdfDataType::Unknown)
{
    this->operator=(rhs);
}

PdfVariant::~PdfVariant()
{
    Clear();
}

void PdfVariant::Clear()
{
    switch (m_DataType)
    {
        case PdfDataType::Array:
        case PdfDataType::Dictionary:
        case PdfDataType::Name:
        case PdfDataType::String:
        case PdfDataType::RawData:
        {
            delete m_Data.Data;
            break;
        }

        case PdfDataType::Reference:
        case PdfDataType::Bool:
        case PdfDataType::Null:
        case PdfDataType::Number:
        case PdfDataType::Real:
        case PdfDataType::Unknown:
        default:
            break;

    }

    m_Data = { };
}

void PdfVariant::Write(PdfOutputDevice& device, PdfWriteMode writeMode,
    const PdfEncrypt* encrypt) const
{
    switch (m_DataType)
    {
        case PdfDataType::Bool:
        {
            if ((writeMode & PdfWriteMode::Compact) == PdfWriteMode::Compact)
                device.Write(" ", 1); // Write space before true or false

            if (m_Data.Bool)
                device.Write("true", 4);
            else
                device.Write("false", 5);
            break;
        }
        case PdfDataType::Number:
        {
            if ((writeMode & PdfWriteMode::Compact) == PdfWriteMode::Compact)
                device.Write(" ", 1); // Write space before numbers

            device.Print("%" PDF_FORMAT_INT64, m_Data.Number);
            break;
        }
        case PdfDataType::Real:
        {
            if ((writeMode & PdfWriteMode::Compact) == PdfWriteMode::Compact)
                device.Write(" ", 1); // Write space before numbers

            // Use ostringstream, so that locale does not matter
            // NOTE: Don't use printf() formatting! It may write the number
            // way that is incompatible in PDF
            ostringstream oss;
            PdfLocaleImbue(oss);
            oss << std::fixed << m_Data.Real;
            string copy = oss.str();
            size_t len = copy.size();

            if ((writeMode & PdfWriteMode::Compact) == PdfWriteMode::Compact &&
                copy.find('.') != string::npos)
            {
                const char* str = copy.c_str();
                while (str[len - 1] == '0')
                    len--;

                if (str[len - 1] == '.')
                    len--;

                if (len == 0)
                {
                    device.Write("0", 1);
                    break;
                }
            }

            device.Write(copy.c_str(), len);
            break;
        }
        case PdfDataType::Reference:
            m_Data.Reference.Write(device, writeMode, encrypt);
            break;
        case PdfDataType::String:
        case PdfDataType::Name:
        case PdfDataType::Array:
        case PdfDataType::Dictionary:
        case PdfDataType::RawData:
            if (m_Data.Data == nullptr)
                PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

            m_Data.Data->Write(device, writeMode, encrypt);
            break;
        case PdfDataType::Null:
        {
            if ((writeMode & PdfWriteMode::Compact) == PdfWriteMode::Compact)
            {
                device.Write(" ", 1); // Write space before null
            }

            device.Print("null");
            break;
        }
        case PdfDataType::Unknown:
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
            break;
        }
    };
}

void PdfVariant::ToString(string& data, PdfWriteMode writeMode) const
{
    ostringstream out;
    // We don't need to this stream with the safe PDF locale because
    // PdfOutputDevice will do so for us.
    PdfStreamOutputDevice device(out);

    this->Write(device, writeMode, nullptr);

    data = out.str();
}

const PdfVariant& PdfVariant::operator=(const PdfVariant& rhs)
{
    Clear();

    m_DataType = rhs.m_DataType;

    switch (m_DataType)
    {
        case PdfDataType::Array:
        {
            m_Data.Data = new PdfArray(*static_cast<PdfArray*>(rhs.m_Data.Data));
            break;
        }
        case PdfDataType::Dictionary:
        {
            m_Data.Data = new PdfDictionary(*static_cast<PdfDictionary*>(rhs.m_Data.Data));
            break;
        }
        case PdfDataType::Name:
        {
            m_Data.Data = new PdfName(*static_cast<PdfName*>(rhs.m_Data.Data));
            break;
        }
        case PdfDataType::String:
        {
            m_Data.Data = new PdfString(*static_cast<PdfString*>(rhs.m_Data.Data));
            break;
        }

        case PdfDataType::RawData:
        {
            m_Data.Data = new PdfData(*static_cast<PdfData*>(rhs.m_Data.Data));
            break;
        }
        case PdfDataType::Reference:
        case PdfDataType::Bool:
        case PdfDataType::Null:
        case PdfDataType::Number:
        case PdfDataType::Real:
            m_Data = rhs.m_Data;
            break;

        case PdfDataType::Unknown:
        default:
            break;
    };

    return *this;
}

const char* PdfVariant::GetDataTypeString() const
{
    switch (m_DataType)
    {
        case PdfDataType::Bool:
            return "Bool";
        case PdfDataType::Number:
            return "Number";
        case PdfDataType::Real:
            return "Real";
        case PdfDataType::String:
            return "String";
        case PdfDataType::Name:
            return "Name";
        case PdfDataType::Array:
            return "Array";
        case PdfDataType::Dictionary:
            return "Dictionary";
        case PdfDataType::Null:
            return "Null";
        case PdfDataType::Reference:
            return "Reference";
        case PdfDataType::RawData:
            return "RawData";
        case PdfDataType::Unknown:
            return "Unknown";
        default:
            return "INVALID_TYPE_ENUM";
    }
}

bool PdfVariant::operator==(const PdfVariant& rhs) const
{
    if (this == &rhs)
        return true;

    switch (m_DataType)
    {
        case PdfDataType::Bool:
        {
            bool value;
            if (rhs.TryGetBool(value))
                return m_Data.Bool == value;
            else
                return false;
        }
        case PdfDataType::Number:
        {
            int64_t value;
            if (rhs.TryGetNumber(value))
                return m_Data.Number == value;
            else
                return false;
        }
        case PdfDataType::Real:
        {
            // NOTE: Real type equality semantics is strict
            double value;
            if (rhs.TryGetRealStrict(value))
                return m_Data.Real == value;
            else
                return false;
        }
        case PdfDataType::Reference:
        {
            PdfReference value;
            if (rhs.TryGetReference(value))
                return m_Data.Reference == value;
            else
                return false;
        }
        case PdfDataType::String:
        {
            const PdfString* value;
            if (rhs.tryGetString(value))
                return *(PdfString*)m_Data.Data == *value;
            else
                return false;
        }
        case PdfDataType::Name:
        {
            const PdfName* value;
            if (rhs.tryGetName(value))
                return *(PdfName*)m_Data.Data == *value;
            else
                return false;
        }
        case PdfDataType::Array:
        {
            const PdfArray* value;
            if (rhs.TryGetArray(value))
                return *(PdfArray*)m_Data.Data == *value;
            else
                return false;
        }
        case PdfDataType::Dictionary:
        {
            const PdfDictionary* value;
            if (rhs.TryGetDictionary(value))
                return *(PdfDictionary*)m_Data.Data == *value;
            else
                return false;
        }
        case PdfDataType::RawData:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Equality not yet implemented for RawData");
        case PdfDataType::Null:
            return m_DataType == PdfDataType::Null;
        case PdfDataType::Unknown:
            return false;
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
    }
}

bool PdfVariant::operator!=(const PdfVariant& rhs) const
{
    if (this != &rhs)
        return true;

    switch (m_DataType)
    {
        case PdfDataType::Bool:
        {
            bool value;
            if (rhs.TryGetBool(value))
                return m_Data.Bool != value;
            else
                return true;
        }
        case PdfDataType::Number:
        {
            int64_t value;
            if (rhs.TryGetNumber(value))
                return m_Data.Number != value;
            else
                return true;
        }
        case PdfDataType::Real:
        {
            // NOTE: Real type equality semantics is strict
            double value;
            if (rhs.TryGetRealStrict(value))
                return m_Data.Real != value;
            else
                return true;
        }
        case PdfDataType::Reference:
        {
            PdfReference value;
            if (rhs.TryGetReference(value))
                return m_Data.Reference != value;
            else
                return true;
        }
        case PdfDataType::String:
        {
            const PdfString* value;
            if (rhs.tryGetString(value))
                return *(PdfString*)m_Data.Data != *value;
            else
                return true;
        }
        case PdfDataType::Name:
        {
            const PdfName* value;
            if (rhs.tryGetName(value))
                return *(PdfName*)m_Data.Data != *value;
            else
                return true;
        }
        case PdfDataType::Array:
        {
            const PdfArray* value;
            if (rhs.TryGetArray(value))
                return *(PdfArray*)m_Data.Data != *value;
            else
                return true;
        }
        case PdfDataType::Dictionary:
        {
            const PdfDictionary* value;
            if (rhs.TryGetDictionary(value))
                return *(PdfDictionary*)m_Data.Data != *value;
            else
                return true;
        }
        case PdfDataType::RawData:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Disequality not yet implemented for RawData");
        case PdfDataType::Null:
            return m_DataType != PdfDataType::Null;
        case PdfDataType::Unknown:
            return true;
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
    }
}

bool PdfVariant::GetBool() const
{
    bool ret;
    if (!TryGetBool(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetBool(bool& value) const
{
    if (m_DataType != PdfDataType::Bool)
    {
        value = false;
        return false;
    }

    value = m_Data.Bool;
    return true;
}

int64_t PdfVariant::GetNumberLenient() const
{
    int64_t ret;
    if (!TryGetNumberLenient(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetNumberLenient(int64_t& value) const
{
    if (!(m_DataType == PdfDataType::Number
        || m_DataType == PdfDataType::Real))
    {
        value = 0;
        return false;
    }

    if (m_DataType == PdfDataType::Real)
        value = static_cast<int64_t>(std::round(m_Data.Real));
    else
        value = m_Data.Number;

    return true;
}

int64_t PdfVariant::GetNumber() const
{
    int64_t ret;
    if (!TryGetNumber(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return m_Data.Number;
}

bool PdfVariant::TryGetNumber(int64_t& value) const
{
    if (m_DataType != PdfDataType::Number)
    {
        value = 0;
        return false;
    }

    value = m_Data.Number;
    return true;
}

double PdfVariant::GetReal() const
{
    double ret;
    if (!TryGetReal(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetReal(double& value) const
{
    if (!(m_DataType == PdfDataType::Real
        || m_DataType == PdfDataType::Number))
    {
        value = 0;
        return false;
    }

    if (m_DataType == PdfDataType::Number)
        value = static_cast<double>(m_Data.Number);
    else
        value = m_Data.Real;

    return true;
}

double PdfVariant::GetRealStrict() const
{
    double ret;
    if (!TryGetRealStrict(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return m_Data.Real;
}

bool PdfVariant::TryGetRealStrict(double& value) const
{
    if (m_DataType != PdfDataType::Real)
    {
        value = 0;
        return false;
    }

    value = m_Data.Real;
    return true;
}

const PdfString& PdfVariant::GetString() const
{
    const PdfString* ret;
    if (!tryGetString(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetString(PdfString& str) const
{
    const PdfString* ret;
    if (!tryGetString(ret))
    {
        str = { };
        return false;
    }

    str = *ret;
    return true;
}

bool PdfVariant::tryGetString(const PdfString*& str) const
{
    if (m_DataType != PdfDataType::String)
    {
        str = nullptr;
        return false;
    }

    str = (PdfString*)m_Data.Data;
    return true;
}

const PdfName& PdfVariant::GetName() const
{
    const PdfName* ret;
    if (!tryGetName(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetName(PdfName& name) const
{
    const PdfName* ret;
    if (!tryGetName(ret))
    {
        name = { };
        return false;
    }

    name = *ret;
    return true;
}

bool PdfVariant::tryGetName(const PdfName*& name) const
{
    if (m_DataType != PdfDataType::Name)
    {
        name = nullptr;
        return false;
    }

    name = (PdfName*)m_Data.Data;
    return true;
}

PdfReference PdfVariant::GetReference() const
{
    PdfReference ret;
    if (!TryGetReference(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetReference(PdfReference& ref) const
{
    if (m_DataType != PdfDataType::Reference)
    {
        ref = PdfReference();
        return false;
    }

    ref = m_Data.Reference;
    return true;
}

const PdfArray& PdfVariant::GetArray() const
{
    PdfArray* ret;
    if (!tryGetArray(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

PdfArray& PdfVariant::GetArray()
{
    PdfArray* ret;
    if (!tryGetArray(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetArray(const PdfArray*& arr) const
{
    return tryGetArray(const_cast<PdfArray*&>(arr));
}

bool PdfVariant::TryGetArray(PdfArray*& arr)
{
    return tryGetArray(arr);
}

const PdfDictionary& PdfVariant::GetDictionary() const
{
    PdfDictionary* ret;
    if (!tryGetDictionary(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

PdfDictionary& PdfVariant::GetDictionary()
{
    PdfDictionary* ret;
    if (!tryGetDictionary(ret))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetDictionary(const PdfDictionary*& dict) const
{
    return tryGetDictionary(const_cast<PdfDictionary*&>(dict));
}

bool PdfVariant::TryGetDictionary(PdfDictionary*& dict)
{
    return tryGetDictionary(dict);
}

bool PdfVariant::tryGetDictionary(PdfDictionary*& dict) const
{
    if (m_DataType != PdfDataType::Dictionary)
    {
        dict = nullptr;
        return false;
    }

    dict = (PdfDictionary*)m_Data.Data;
    return true;
}

bool PdfVariant::tryGetArray(PdfArray*& arr) const
{
    if (m_DataType != PdfDataType::Array)
    {
        arr = nullptr;
        return false;
    }

    arr = (PdfArray*)m_Data.Data;
    return true;
}

void PdfVariant::SetBool(bool value)
{
    if (m_DataType != PdfDataType::Bool)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    m_Data.Bool = value;
}

void PdfVariant::SetNumber(int64_t value)
{
    if (!(m_DataType == PdfDataType::Number
        || m_DataType == PdfDataType::Real))
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }

    if (m_DataType == PdfDataType::Real)
        m_Data.Real = static_cast<double>(value);
    else
        m_Data.Number = value;
}

void PdfVariant::SetReal(double value)
{
    if (!(m_DataType == PdfDataType::Real
        || m_DataType == PdfDataType::Number))
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }

    if (m_DataType == PdfDataType::Number)
        m_Data.Number = static_cast<int64_t>(std::round(value));
    else
        m_Data.Real = value;
}

void PdfVariant::SetName(const PdfName& name)
{
    if (m_DataType != PdfDataType::Name)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    *((PdfName*)m_Data.Data) = name;
}

void PdfVariant::SetString(const PdfString& str)
{
    if (m_DataType != PdfDataType::String)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    *((PdfString*)m_Data.Data) = str;
}

void PdfVariant::SetReference(const PdfReference& ref)
{
    if (m_DataType != PdfDataType::Reference)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    m_Data.Reference = ref;
}

bool PdfVariant::IsBool() const
{
    return m_DataType == PdfDataType::Bool;
}

bool PdfVariant::IsNumber() const
{
    return m_DataType == PdfDataType::Number;
}

bool PdfVariant::IsRealStrict() const
{
    return m_DataType == PdfDataType::Real;
}

bool PdfVariant::IsNumberOrReal() const
{
    return m_DataType == PdfDataType::Number || m_DataType == PdfDataType::Real;
}

bool PdfVariant::IsString() const
{
    return m_DataType == PdfDataType::String;
}

bool PdfVariant::IsName() const
{
    return m_DataType == PdfDataType::Name;
}

bool PdfVariant::IsArray() const
{
    return m_DataType == PdfDataType::Array;
}

bool PdfVariant::IsDictionary() const
{
    return m_DataType == PdfDataType::Dictionary;
}

bool PdfVariant::IsRawData() const
{
    return m_DataType == PdfDataType::RawData;
}

bool PdfVariant::IsNull() const
{
    return m_DataType == PdfDataType::Null;
}

bool PdfVariant::IsReference() const
{
    return m_DataType == PdfDataType::Reference;
}