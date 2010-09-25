/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _PDF_PAGE_H_
#define _PDF_PAGE_H_

#include "PdfDefines.h"

#include "PdfArray.h"
#include "PdfAnnotation.h"
#include "PdfCanvas.h"
#include "PdfContents.h"
#include "PdfElement.h"
#include "PdfField.h"
#include "PdfRect.h"

namespace PoDoFo {

class PdfDocument;
class PdfDictionary;
class PdfVecObjects;

typedef std::map<PdfReference,PdfAnnotation*> TMapAnnotation;
typedef TMapAnnotation::iterator              TIMapAnnotation;
typedef TMapAnnotation::const_iterator        TCIMapAnnotation;

/** PdfPage is one page in the pdf document. 
 *  It is possible to draw on a page using a PdfPainter object.
 *  Every document needs at least one page.
 */
class PODOFO_API PdfPage : public PdfElement, public PdfCanvas {
 public:
    /** Create a new PdfPage object.
     *  \param rSize a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param pParent add the page to this parent
     */
    PdfPage( const PdfRect & rSize, PdfDocument* pParent );

    /** Create a new PdfPage object.
     *  \param rSize a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param pParent add the page to this parent
     */
    PdfPage( const PdfRect & rSize, PdfVecObjects* pParent );
 
    /** Create a PdfPage based on an existing PdfObject
     *  \param pObject an existing PdfObject
     *  \param listOfParents a list of PdfObjects that are
     *                       parents of this page and can be 
     *                       queried for inherited attributes.
     *                       The last object in the list is the
     *                       most direct parent of this page.
     */
    PdfPage( PdfObject* pObject, const std::deque<PdfObject*> & listOfParents );

    virtual ~PdfPage();

    /** Get the current page size in PDF Units
     *  \returns a PdfRect containing the page size available for drawing
     */
    inline virtual const PdfRect GetPageSize() const;

    // added by Petr P. Petrov 21 Febrary 2010
    /** Set the current page width in PDF Units
     *
     * \returns true if successfull, false otherwise
     *
     */
    bool SetPageWidth(int newWidth);

    // added by Petr P. Petrov 21 Febrary 2010
    /** Set the current page height in PDF Units
     *
     * \returns true if successfull, false otherwise
     *
     */
    bool SetPageHeight(int newHeight);

    /** Set the trimbox in PDF Units
     *  \param rSize a PdfRect specifying the trimbox of the page (i.e the /TrimBox key) in PDF units
     */
	void SetTrimBox( const PdfRect & rSize );

	/** Page number inside of the document. The  first page
     *  has the number 1, the last page has the number 
     *  PdfPagesTree:GetTotalNumberOfPages()
     *
     *  \returns the number of the page inside of the document
     *
     *  \see PdfPagesTree:GetTotalNumberOfPages()
     */
    unsigned int GetPageNumber() const;

    /** Creates a PdfRect with the page size as values which is needed to create a PdfPage object
     *  from an enum which are defined for a few standard page sizes.
     *
     *  \param ePageSize the page size you want
     *  \param bLandscape create a landscape pagesize instead of portrait (by exchanging width and height)
     *  \returns a PdfRect object which can be passed to the PdfPage constructor
     */
    static PdfRect CreateStandardPageSize( const EPdfPageSize ePageSize, bool bLandscape = false );

    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfObject* GetContents() const;

    /** Get access an object that you can use to ADD drawing to.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfObject* GetContentsForAppending() const;

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    inline virtual PdfObject* GetResources() const;

    /** Get the current MediaBox (physical page size) in PDF units.
     *  \returns PdfRect the page box
     */
    virtual const PdfRect GetMediaBox() const { return GetPageBox( "MediaBox" ); }

    /** Get the current CropBox (visible page size) in PDF units.
     *  \returns PdfRect the page box
     */
    virtual const PdfRect GetCropBox() const { return GetPageBox( "CropBox" ); }

    /** Get the current TrimBox (cut area) in PDF units.
     *  \returns PdfRect the page box
     */
    virtual const PdfRect GetTrimBox() const { return GetPageBox( "TrimBox" ); }

    /** Get the current BleedBox (extra area for printing purposes) in PDF units.
     *  \returns PdfRect the page box
     */
    virtual const PdfRect GetBleedBox() const { return GetPageBox( "BleedBox" ); }

    /** Get the current ArtBox in PDF units.
     *  \returns PdfRect the page box
     */
    virtual const PdfRect GetArtBox() const { return GetPageBox( "ArtBox" ); }

    /** Get the current page rotation (if any).
     *  \returns int 0, 90, 180 or 270
     */
    virtual int GetRotation() const;
        
    /** Get the number of annotations associated with this page
     * \ returns int number of annotations
     */
    virtual int GetNumAnnots() const;

    /** Create a new annotation to this page.
     *  \param eType the type of the annotation
     *  \param rRect rectangle of the annotation on the page
     *
     *  \returns the annotation object which is owned by the PdfPage
     */
    PdfAnnotation* CreateAnnotation( EPdfAnnotation eType, const PdfRect & rRect );

    /** Get the annotation with index index of the current page.
     *  \param index the index of the annotation to retrieve
     *
     *  \returns a annotation object. The annotation object is owned by the PdfPage.
     *
     *  \see GetNumAnnots
     */
    PdfAnnotation* GetAnnotation( int index );

    /** Delete the annotation with index index from this page.
     *  \param index the index of the annotation to delete
     *
     *  \see GetNumAnnots
     */
    void DeleteAnnotation( int index );

    /** Delete the annotation object with reference ref from this page.
     *  \param ref the reference of an annotation object of this page.
     *
     *  \see GetNumAnnots
     */
    void DeleteAnnotation( const PdfReference & ref );

    /** 
     * \returns the number of PdfFields on this page.
     */
    int GetNumFields() const;

    /** Get a PdfField with a certain index.
     *  \param index of the PdfField (must be smaller than GetNumFields() )
     *
     *  \see GetNumFields
     *
     *  \returns a PdfField
     */
    PdfField GetField( int index );

    /** Get a PdfField with a certain index.
     *  \param index of the PdfField (must be smaller than GetNumFields() )
     *
     *  \see GetNumFields
     *
     *  \returns a constP dfField
     */
    const PdfField GetField( int index ) const;

    /** Get an element from the pages resources dictionary,
     *  using a type (category) and a key.
     *
     *  \param rType the type of resource to fetch (e.g. /Font, or /XObject)
     *  \param rKey the key of the resource
     *
     *  \returns the object of the resource or NULL if it was not found
     */
    PdfObject* GetFromResources( const PdfName & rType, const PdfName & rKey );

 private:

    /**
     * Initialize a new page object.
     * m_pContents must be initialized before calling this!
     *
     * @param rSize page size
     */
    void InitNewPage( const PdfRect & rSize );

    /**
     * Create the internal PdfContents object.
     * Call this before accessing m_pContents as
     * the object is only created if needed.
     */
    void CreateContents();

    /** Get the bounds of a specified page box in PDF units.
     * This function is internal, since there are wrappers for all standard boxes
     *  \returns PdfRect the page box
     */
    const PdfRect GetPageBox( const char* inBox ) const;
    
    /** Private method for getting a key value that could be inherited (such as the boxes, resources, etc.)
     *  \returns PdfVariant - the result of the key fetching
     */
    const PdfObject* GetInheritedKeyFromObject( const char* inKey, const PdfObject* inObject ) const; 

    /** Get the annotations array.
     *  \param bCreate if true the annotations array is created 
     *                 if it does not exist.
     *  \returns the annotations array or NULL if none exists.
     */
    PdfObject* GetAnnotationsArray( bool bCreate = false ) const;

 private:
    PdfContents*   m_pContents;
    PdfObject*     m_pResources;

    TMapAnnotation m_mapAnnotations;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfPage::GetResources() const
{
    return m_pResources;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfRect PdfPage::GetPageSize() const
{
    return this->GetMediaBox();
}

};

#endif // _PDF_PAGE_H_
