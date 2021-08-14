/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_INFO_H
#define PDF_INFO_H

#include "PdfDefines.h"

#include "PdfName.h"
#include "PdfDate.h"
#include "PdfElement.h"

namespace mm
{

class PdfString;

/** This class provides access to the documents
 *  info dictionary, which provides information
 *  about the PDF document.
 */
class PDFMM_API PdfInfo final : public PdfElement
{
public:

    /** Create a new PdfInfo object
     *  \param parent the parent of this object
     *  \param initial which information should be
     *         writting initially to the information dictionary
     */
    PdfInfo(PdfDocument& doc,
        PdfInfoInitial initial = PdfInfoInitial::WriteCreationTime | PdfInfoInitial::WriteProducer);

    /** Create a PdfInfo object from an existing
     *  object in the PDF file.
     *  \param obj must be an info dictionary.
     *  \param initial which information should be
     *         writting initially to the information
     */
    PdfInfo(PdfObject& obj, PdfInfoInitial initial = PdfInfoInitial::None);

    /** Set the author of the document.
     *  \param author author
     */
    void SetAuthor(const PdfString& author);

    /** Get the author of the document
     *  \returns the author
     */
    std::optional<PdfString> GetAuthor() const;

    /** Set the creator of the document.
     *  Typically the name of the application using the library.
     *  \param creator creator
     */
    void SetCreator(const PdfString& creator);

    /** Get the creator of the document
     *  \returns the creator
     */
    std::optional<PdfString> GetCreator() const;

    /** Set keywords for this document
     *  \param keywords a list of keywords
     */
    void SetKeywords(const PdfString& keywords);

    /** Get the keywords of the document
     *  \returns the keywords
     */
    std::optional<PdfString> GetKeywords() const;

    /** Set the subject of the document.
     *  \param subject subject
     */
    void SetSubject(const PdfString& subject);

    /** Get the subject of the document
     *  \returns the subject
     */
    std::optional<PdfString> GetSubject() const;

    /** Set the title of the document.
     *  \param title title
     */
    void SetTitle(const PdfString& title);

    /** Get the title of the document
     *  \returns the title
     */
    std::optional<PdfString> GetTitle() const;

    // Peter Petrov 27 April 2008
    /** Set the producer of the document.
     *  \param producer producer
     */
    void SetProducer(const PdfString& producer);

    // Peter Petrov 27 April 2008
    /** Get the producer of the document
     *  \returns the producer
     */
    std::optional<PdfString> GetProducer() const;

    /** Set the trapping state of the document.
     *  \param trapped trapped
     */
    void SetTrapped(const PdfName& trapped);

    /** Get the trapping state of the document
     *  \returns the title
     */
    const PdfName& GetTrapped() const;

    /** Get creation date of document
     *  \return creation date
     */
    PdfDate GetCreationDate() const;

    /** Get modification date of document
     *  \return modification date
     */
    PdfDate GetModDate() const;

    /** Set custom info key.
     * \param name Name of the key.
     * \param value Value of the key.
     */
    void SetCustomKey(const PdfName& name, const PdfString& value);
private:
    /** Add the initial document information to the dictionary.
     *  \param initial which information should be
     *         writting initially to the information
     */
    void Init(PdfInfoInitial initial);

    /** Get a value from the info dictionary as name
     *  \para name the key to fetch from the info dictionary
     *  \return a value from the info dictionary
     */
    std::optional<PdfString> GetStringFromInfoDict(const PdfName& name) const;

    /** Get a value from the info dictionary as name
    *  \para name the key to fetch from the info dictionary
    *  \return a value from the info dictionary
    */
    const PdfName& GetNameFromInfoDict(const PdfName& name) const;

};

};

#endif // PDF_INFO_H