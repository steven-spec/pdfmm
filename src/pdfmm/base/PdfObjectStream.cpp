/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfObjectStream.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfFilter.h"
#include "PdfInputStream.h"
#include "PdfOutputStream.h"
#include "PdfOutputDevice.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

enum PdfFilterType PdfObjectStream::DefaultFilter = PdfFilterType::FlateDecode;

PdfObjectStream::PdfObjectStream(PdfObject& parent)
    : m_Parent(&parent), m_Append(false)
{
}

PdfObjectStream::~PdfObjectStream() { }

void PdfObjectStream::GetFilteredCopy(PdfOutputStream& stream) const
{
    PdfFilterList filters = PdfFilterFactory::CreateFilterList(*m_Parent);
    if (filters.size() != 0)
    {
        auto decodeStream = PdfFilterFactory::CreateDecodeStream(filters, stream,
            &m_Parent->GetDictionary());

        decodeStream->Write(const_cast<char*>(this->GetInternalBuffer()), this->GetInternalBufferSize());
        decodeStream->Close();
    }
    else
    {
        stream.Write(this->GetInternalBuffer(), this->GetInternalBufferSize());
    }
}

charbuff PdfObjectStream::GetFilteredCopy() const
{
    charbuff ret;
    PdfStringOutputStream stream(ret);
    GetFilteredCopy(stream);
    return ret;
}

void PdfObjectStream::MoveTo(PdfObject& obj)
{
    PDFMM_RAISE_LOGIC_IF(m_Append, "EndAppend() should be called before moving the stream");
    obj.MoveStreamFrom(*m_Parent);
}

void PdfObjectStream::GetFilteredCopy(unique_ptr<char[]>& buffer, size_t& len) const
{
    PdfFilterList filters = PdfFilterFactory::CreateFilterList(*m_Parent);
    PdfMemoryOutputStream  stream;
    if (filters.size())
    {
        auto decodeStream = PdfFilterFactory::CreateDecodeStream(filters, stream,
            &m_Parent->GetDictionary());

        decodeStream->Write(this->GetInternalBuffer(), this->GetInternalBufferSize());
        decodeStream->Close();
    }
    else
    {
        stream.Write(this->GetInternalBuffer(), this->GetInternalBufferSize());
        stream.Close();
    }

    buffer = stream.TakeBuffer(len);
}

PdfObjectStream& PdfObjectStream::operator=(const PdfObjectStream& rhs)
{
    CopyFrom(rhs);
    return (*this);
}

void PdfObjectStream::CopyFrom(const PdfObjectStream& rhs)
{
    PdfMemoryInputStream stream({ rhs.GetInternalBuffer(), rhs.GetInternalBufferSize() });
    this->SetRawData(stream);
}

void PdfObjectStream::EnsureAppendClosed()
{
    PDFMM_RAISE_LOGIC_IF(m_Append, "EndAppend() should be called after appending to stream");
}

void PdfObjectStream::Set(const bufferview& buffer, const PdfFilterList& filters)
{
    Set(buffer.data(), buffer.size(), filters);
}

void PdfObjectStream::Set(const char* buffer, size_t len, const PdfFilterList& filters)
{
    if (len == 0)
        return;

    this->BeginAppend(filters);
    this->append(buffer, len);
    this->endAppend();
}

void PdfObjectStream::Set(const bufferview& buffer)
{
    Set(buffer.data(), buffer.size());
}

void PdfObjectStream::Set(const char* buffer, size_t len)
{
    if (len == 0)
        return;

    this->BeginAppend();
    this->append(buffer, len);
    this->endAppend();
}

void PdfObjectStream::Set(PdfInputStream& stream)
{
    PdfFilterList filters;

    if (DefaultFilter != PdfFilterType::None)
        filters.push_back(DefaultFilter);

    this->Set(stream, filters);
}

void PdfObjectStream::Set(PdfInputStream& stream, const PdfFilterList& filters)
{
    constexpr size_t BUFFER_SIZE = 4096;
    size_t len = 0;
    char buffer[BUFFER_SIZE];

    this->BeginAppend(filters);

    bool eof;
    do
    {
        len = stream.Read(buffer, BUFFER_SIZE, eof);
        this->append(buffer, len);
    } while (!eof);

    this->endAppend();
}

void PdfObjectStream::SetRawData(PdfInputStream& stream, ssize_t len)
{
    SetRawData(stream, len, true);
}

void PdfObjectStream::SetRawData(PdfInputStream& stream, ssize_t len, bool markObjectDirty)
{
    constexpr size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    size_t lenRead;
    PdfFilterList filters;

    // TODO: DS, give begin append a size hint so that it knows
    //       how many data has to be allocated
    this->BeginAppend(filters, true, false, markObjectDirty);
    if (len < 0)
    {
        bool eof;
        do
        {
            lenRead = stream.Read(buffer, BUFFER_SIZE, eof);
            this->append(buffer, lenRead);
        } while (!eof);
    }
    else
    {
        bool eof;
        do
        {
            lenRead = stream.Read(buffer, std::min(BUFFER_SIZE, (size_t)len), eof);
            len -= lenRead;
            this->append(buffer, lenRead);
        } while (len > 0 && !eof);
    }

    this->endAppend();
}

void PdfObjectStream::BeginAppend(bool clearExisting)
{
    PdfFilterList filters;

    if (DefaultFilter != PdfFilterType::None)
        filters.push_back(DefaultFilter);

    this->BeginAppend(filters, clearExisting);
}

void PdfObjectStream::BeginAppend(const PdfFilterList& filters, bool clearExisting, bool deleteFilters)
{
    BeginAppend(filters, clearExisting, deleteFilters, true);
}

void PdfObjectStream::BeginAppend(const PdfFilterList& filters, bool clearExisting, bool deleteFilters, bool markObjectDirty)
{
    PDFMM_RAISE_LOGIC_IF(m_Append, "BeginAppend() failed because EndAppend() was not yet called!");

    if (markObjectDirty)
    {
        // We must make sure the parent will be set dirty. All methods
        // writing to the stream will call this method first
        m_Parent->SetDirty();
    }

    auto document = m_Parent->GetDocument();
    if (document != nullptr)
        document->GetObjects().BeginAppendStream(*this);

    size_t len = 0;
    unique_ptr<char[]> buffer;
    if (!clearExisting && this->GetLength() != 0)
        this->GetFilteredCopy(buffer, len);

    if (filters.size() == 0)
    {
        if (deleteFilters)
            m_Parent->GetDictionary().RemoveKey(PdfName::KeyFilter);
    }
    else if (filters.size() == 1)
    {
        m_Parent->GetDictionary().AddKey(PdfName::KeyFilter,
            PdfName(PdfFilterFactory::FilterTypeToName(filters.front())));
    }
    else // filters.size() > 1
    {
        PdfArray arrFilters;
        for (auto filterType : filters)
            arrFilters.Add(PdfName(PdfFilterFactory::FilterTypeToName(filterType)));

        m_Parent->GetDictionary().AddKey(PdfName::KeyFilter, arrFilters);
    }

    this->BeginAppendImpl(filters);
    m_Append = true;
    if (buffer != nullptr)
        this->append(buffer.get(), len);
}

void PdfObjectStream::EndAppend()
{
    PDFMM_RAISE_LOGIC_IF(!m_Append, "EndAppend() failed because BeginAppend() was not yet called!");
    endAppend();
}

void PdfObjectStream::endAppend()
{
    m_Append = false;
    this->EndAppendImpl();

    PdfDocument* document;
    if ((document = m_Parent->GetDocument()) != nullptr)
        document->GetObjects().EndAppendStream(*this);
}

PdfObjectStream& PdfObjectStream::Append(const string_view& view)
{
    Append(view.data(), view.length());
    return *this;
}

PdfObjectStream& PdfObjectStream::Append(const char* buffer, size_t len)
{
    PDFMM_RAISE_LOGIC_IF(!m_Append, "Append() failed because BeginAppend() was not yet called!");
    if (len == 0)
        return *this;

    append(buffer, len);
    return *this;
}

void PdfObjectStream::append(const char* data, size_t len)
{
    this->AppendImpl(data, len);
}