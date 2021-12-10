/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CONTENT_READER_H
#define PDF_CONTENT_READER_H

#include "PdfXObject.h"
#include "PdfCanvas.h"
#include "PdfData.h"
#include "PdfDictionary.h"
#include "PdfVariantStack.h"
#include "PdfPostScriptTokenizer.h"

namespace mm {

/** Type of the content read from a content stream
 */
enum class PdfContentType
{
    Unknown = 0,
    Operator,          ///< The token is a PDF operator
    ImageDictionary,   ///< Inline image dictionary
    ImageData,         ///< Raw inline image data found between ID and EI tags (see PDF ref section 4.8.6)
    DoXObject,         ///< Issued when a Do operator is found and it is handled by the reader
    EndXObjectForm,    ///< Issued when the end of a XObject form is detected
};

enum class PdfContentWarnings
{
    None = 0,
    InvalidOperator = 1,            ///< Unknown operator or insufficient operand count
    SpuriousStackContent = 2,       ///< Operand count for the operator are more than necessary
    InvalidPostScriptContent = 4,   ///< Invalid PostScript statements found when reading for content
    InvalidXObject = 8,             ///< Invalid or not found XObject
    RecursiveXObject = 16,          ///< Recursive XObject call detected
    MissingEndImage = 32,           ///< Missing end inline image EI operator 
};

/** Content as read from content streams
 */
struct PdfContent
{
    PdfContentType Type = PdfContentType::Unknown;
    PdfContentWarnings Warnings = PdfContentWarnings::None;
    PdfVariantStack Stack;
    PdfOperator Operator = PdfOperator::Unknown;
    std::string_view Keyword;
    PdfDictionary InlineImageDictionary;
    PdfData InlineImageData;
    std::shared_ptr<const PdfXObject> XObject;
};

enum class PdfContentReaderFlags
{
    None = 0,
    ThrowOnWarnings = 1,
    DontFollowXObjects = 2,
};

/** Custom handler for inline images
 * \param imageDict dictionary for the inline image
 * \returns false if EOF 
 */
typedef std::function<bool(const PdfDictionary& imageDict, PdfInputDevice& device)> PdfInlineImageHandler;

struct PdfContentReaderArgs
{
    PdfContentReaderFlags Flags = PdfContentReaderFlags::None;
    PdfInlineImageHandler InlineImageHandler;
};

/** Reader class to read content streams
 */
class PdfContentsReader final
{
public:
    PdfContentsReader(const PdfCanvas& canvas, nullable<const PdfContentReaderArgs&> args = { });

    PdfContentsReader(const std::shared_ptr<PdfInputDevice>& device, nullable<const PdfContentReaderArgs&> args = { });

private:
    PdfContentsReader(const std::shared_ptr<PdfInputDevice>& device, const PdfCanvas* canvas,
        nullable<const PdfContentReaderArgs&> args);

public:
    bool TryReadNext(PdfContent& data);

private:
    void beforeReadReset(PdfContent& content);

    void afterReadClear(PdfContent& content);

    bool tryReadNextContent(PdfContent& content);

    bool tryHandleOperator(PdfContent& content);

    bool tryReadInlineImgDict(PdfContent& content);

    bool tryReadInlineImgData(PdfData& data);

    void tryFollowXObject(PdfContent& content);

    void handleWarnings();

    bool isCalledRecursively(const PdfObject* xobj);

private:
    struct Storage
    {
        PdfPostScriptTokenType PsType;
        std::string_view Keyword;
        PdfVariant Variant;
        PdfName Name;
    };

    struct Input
    {
        std::shared_ptr<const PdfXObject> Form;
        std::shared_ptr<PdfInputDevice> Device;
        const PdfCanvas* Canvas;
    };

private:
    std::vector<Input> m_inputs;
    PdfContentReaderArgs m_args;
    std::shared_ptr<chars> m_buffer;
    PdfPostScriptTokenizer m_tokenizer;
    bool m_readingInlineImgData;  // A state of reading inline image data

    // Temp storage
    Storage m_temp;
};

};

ENABLE_BITMASK_OPERATORS(mm::PdfContentReaderFlags);
ENABLE_BITMASK_OPERATORS(mm::PdfContentWarnings);

#endif // PDF_CONTENT_READER_H
