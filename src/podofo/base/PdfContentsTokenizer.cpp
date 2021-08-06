/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfContentsTokenizer.h"

#include "PdfOutputStream.h"
#include "PdfVecObjects.h"
#include "PdfData.h"
#include "PdfDictionary.h"
#include "PdfCanvasInputDevice.h"

#include <iostream>
#include <list>

using namespace std;
using namespace PoDoFo;

PdfContentsTokenizer::PdfContentsTokenizer(PdfCanvas& canvas)
    : PdfContentsTokenizer(std::make_shared<PdfCanvasInputDevice>(canvas))
{
}

PdfContentsTokenizer::PdfContentsTokenizer(const std::shared_ptr<PdfInputDevice>& device)
    : m_buffer(PdfTokenizer::BufferSize), m_tokenizer(m_buffer), m_device(device), m_readingInlineImgData(false)
{
}

bool PdfContentsTokenizer::tryReadInlineImgDict(PdfDictionary& dict)
{
    EPdfContentsType type;
    string_view keyword;
    PdfVariant variant;
    while (true)
    {
        if (!tryReadNext(type, keyword, variant))
            return false;

        PdfName key;
        switch (type)
        {
            case EPdfContentsType::Keyword:
            {
                // Try to find end of dictionary
                if (keyword == "ID")
                    return true;
                else
                    return false;
            }
            case EPdfContentsType::Variant:
            {
                if (variant.TryGetName(key))
                    break;
                else
                    return false;
            }
            case EPdfContentsType::Unknown:
            {
                return false;
            }
            case EPdfContentsType::ImageDictionary:
            case EPdfContentsType::ImageData:
            {
                throw runtime_error("Unsupported flow");
            }
        }

        if (TryReadNextVariant(variant))
            dict.AddKey(key, PdfObject(variant));
        else
            return false;
    }
}

bool PdfContentsTokenizer::TryReadNext(EPdfContentsType& contentsType, string_view& keyword, PdfVariant& variant)
{
    if (m_readingInlineImgData)
    {
        keyword = { };
        PdfData data;
        if (tryReadInlineImgData(data))
        {
            variant = data;
            contentsType = EPdfContentsType::ImageData;
            m_readingInlineImgData = false;
            return true;
        }
        else
        {
            contentsType = EPdfContentsType::Unknown;
            m_readingInlineImgData = false;
            return false;
        }
    }

    if (!tryReadNext(contentsType, keyword, variant))
    {
        contentsType = EPdfContentsType::Unknown;
        return false;
    }

    if (contentsType == EPdfContentsType::Keyword && keyword == "BI")
    {
        PdfDictionary dict;
        if (tryReadInlineImgDict(dict))
        {
            variant = dict;
            contentsType = EPdfContentsType::ImageDictionary;
            m_readingInlineImgData = true;
            return true;
        }
        else
        {
            contentsType = EPdfContentsType::Unknown;
            return false;
        }
    }

    return true;
}

void PdfContentsTokenizer::ReadNextVariant(PdfVariant& variant)
{
    return m_tokenizer.ReadNextVariant(*m_device, variant);
}

bool PdfContentsTokenizer::TryReadNextVariant(PdfVariant& variant)
{
    return m_tokenizer.TryReadNextVariant(*m_device, variant);
}

bool PdfContentsTokenizer::tryReadNext(EPdfContentsType& type, string_view& keyword, PdfVariant& variant)
{
    EPdfPostScriptTokenType psTokenType;
    bool gotToken = m_tokenizer.TryReadNext(*m_device, psTokenType, keyword, variant);
    if (!gotToken)
    {
        type = EPdfContentsType::Unknown;
        return false;
    }

    switch (psTokenType)
    {
        case EPdfPostScriptTokenType::Keyword:
            type = EPdfContentsType::Keyword;
            break;
        case EPdfPostScriptTokenType::Variant:
            type = EPdfContentsType::Variant;
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidEnumValue, "Invalid token at this context");
    }

    return true;
}

bool PdfContentsTokenizer::tryReadInlineImgData(PdfData& data)
{
    // Consume one whitespace between ID and data
    char ch;
    if (!m_device->TryGetChar(ch))
        return false;

    // Read "EI"
    enum class ReadEIStatus
    {
        ReadE,
        ReadI,
        ReadWhiteSpace
    };

    // NOTE: This is a better version of the previous approach
    // and still is wrong since the Pdf specification is broken
    // with this regard. The dictionary should have a /Length
    // key with the length of the data, and it's a requirement
    // in Pdf 2.0 specification (ISO 32000-2). To handle better
    // the situation the only approach would be to use more
    // comprehensive heuristic, similarly to what pdf.js does
    ReadEIStatus status = ReadEIStatus::ReadE;
    unsigned readCount = 0;
    while (m_device->TryGetChar(ch))
    {
        switch (status)
        {
            case ReadEIStatus::ReadE:
            {
                if (ch == 'E')
                    status = ReadEIStatus::ReadI;

                break;
            }
            case ReadEIStatus::ReadI:
            {
                if (ch == 'I')
                    status = ReadEIStatus::ReadWhiteSpace;
                else
                    status = ReadEIStatus::ReadE;

                break;
            }
            case ReadEIStatus::ReadWhiteSpace:
            {
                if (PdfTokenizer::IsWhitespace(ch))
                {
                    data = string_view(m_buffer.GetBuffer(), readCount - 2);
                    return true;
                }
                else
                    status = ReadEIStatus::ReadE;

                break;
            }
        }

        if (readCount == m_buffer.GetSize())
        {
            // image is larger than buffer => resize buffer
            m_buffer.Resize(m_buffer.GetSize() * 2);
        }

        m_buffer.GetBuffer()[readCount] = ch;
        readCount++;
    }

    return false;
}
