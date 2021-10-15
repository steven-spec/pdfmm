/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_INDIRECT_OBJECT_LIST_H
#define PDF_INDIRECT_OBJECT_LIST_H

#include <set>
#include <list>
#include "PdfDefines.h"
#include "PdfReference.h"

namespace mm {

class PdfDocument;
class PdfObject;
class PdfStream;
class PdfVariant;

typedef std::deque<PdfReference> PdfReferenceList;

/** A list of PdfObjects that constitutes the indirect object list
 *  of the document
 *  The PdfParser will read the PdfFile into memory and create
 *  a PdfIndirectObjectList of all dictionaries found in the PDF file.
 *
 *  The PdfWriter class contrary creates a PdfIndirectObjectList internally
 *  and writes it to a PDF file later with an appropriate table of
 *  contents.
 *
 *  This class contains also advanced functions for searching of PdfObject's
 *  in a PdfIndirectObjectList.
 */
class PDFMM_API PdfIndirectObjectList final
{
    friend class PdfWriter;
    friend class PdfDocument;
    friend class PdfParser;
    friend class PdfObjectStreamParser;

private:
    static bool CompareObject(const PdfObject* p1, const PdfObject* p2);
    static bool CompareReference(const PdfObject* obj, const PdfReference& ref);

private:
    typedef std::set<PdfObject*, decltype(CompareObject)*> ObjectList;

public:
    // An incomplete set of container typedefs, just enough to handle
    // the begin() and end() methods we wrap from the internal vector.
    // TODO: proper wrapper iterator class.
    typedef ObjectList::const_iterator iterator;

    /** Every observer of PdfIndirectObjectList has to implement this interface.
     */
    class PDFMM_API Observer
    {
        friend class PdfIndirectObjectList;
    public:
        virtual ~Observer() { }

        virtual void WriteObject(const PdfObject& obj) = 0;

        /** Called whenever appending to a stream is started.
         *  \param stream the stream object the user currently writes to.
         */
        virtual void BeginAppendStream(const PdfStream& stream) = 0;

        /** Called whenever appending to a stream has ended.
         *  \param stream the stream object the user currently writes to.
         */
        virtual void EndAppendStream(const PdfStream& stream) = 0;

        virtual void Finish() = 0;
    };

    /** This class is used to implement stream factories in pdfmm.
     */
    class PDFMM_API StreamFactory
    {
    public:
        virtual ~StreamFactory() { }

        /** Creates a stream object
         *
         *  \param parent parent object
         *
         *  \returns a new stream object
         */
        virtual PdfStream* CreateStream(PdfObject& parent) = 0;
    };

public:
    ~PdfIndirectObjectList();

    /** Enable/disable object numbers re-use.
     *  By default object numbers re-use is enabled.
     *
     *  \param canReuseObjectNumbers if true, free object numbers can be re-used when creating new objects.
     *
     *  If set to false, the list of free object numbers is automatically cleared.
     */
    void SetCanReuseObjectNumbers(bool canReuseObjectNumbers);

    /** Removes all objects from the vector
     *  and resets it to the default state.
     *
     *  If SetAutoDelete is true all objects are deleted.
     *  All observers are removed from the vector.
     *
     *  \see SetAutoDelete
     *  \see IsAutoDelete
     */
    void Clear();

    /**
     *  \returns the size of the internal vector
     */
    unsigned GetSize() const;

    /**
     *  \returns the highest object number in the vector
     */
    unsigned GetObjectCount() const { return m_ObjectCount; }

    /** Finds the object with the given reference
     *  and returns a pointer to it if it is found. Throws a PdfError
     *  exception with error code ePdfError_NoObject if no object was found
     *  \param ref the object to be found
     *  \returns the found object
     *  \throws PdfError(ePdfError_NoObject)
     */
    PdfObject& MustGetObject(const PdfReference& ref) const;

    /** Finds the object with the given reference
     *  and returns a pointer to it if it is found.
     *  \param ref the object to be found
     *  \returns the found object or nullptr if no object was found.
     */
    PdfObject* GetObject(const PdfReference& ref) const;

    /** Remove the object with the given object and generation number from the list
     *  of objects.
     *  The object is returned if it was found. Otherwise nullptr is returned.
     *  The caller has to delete the object by himself.
     *
     *  \param ref the object to be found
     *  \param bMarkAsFree if true the removed object reference is marked as free object
     *                     you will always want to have this true
     *                     as invalid PDF files can be generated otherwise
     *  \returns The removed object.
     */
    std::unique_ptr<PdfObject> RemoveObject(const PdfReference& ref, bool markAsFree = true);

    /** Remove the object with the iterator it from the vector and return it
     *  \param it the object to remove
     *  \returns the removed object
     */
    std::unique_ptr<PdfObject> RemoveObject(const iterator& it);

    /** Creates a new object and inserts it into the vector.
     *  This function assigns the next free object number to the PdfObject.
     *
     *  \param type optional value of the /Type key of the object
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfObject* CreateDictionaryObject(const std::string_view& type = { });

    /** Creates a new object (of type rVariants) and inserts it into the vector.
     *  This function assigns the next free object number to the PdfObject.
     *
     *  \param variant value of the PdfObject
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfObject* CreateObject(const PdfVariant& variant);

    /** Attach a new observer
     *  \param pObserver to attach
     */
    void Attach(Observer* observer);

    /** Detach an observer.
     *
     *  \param pObserver observer to detach
     */
    void Detach(Observer* observer);

    /** Sets a StreamFactory which is used whenever CreateStream is called.
     *
     *  \param pFactory a stream factory or nullptr to reset to the default factory
     */
    void SetStreamFactory(StreamFactory* factory);

    /** Creates a stream object
     *  This method is a factory for PdfStream objects.
     *
     *  \param parent parent object
     *
     *  \returns a new stream object
     */
    PdfStream* CreateStream(PdfObject& parent);

    /** Can be called to force objects to be written to disk.
     *
     *  \param obj a PdfObject that should be written to disk.
     */
    void WriteObject(PdfObject& obj);

    /** Call whenever a document is finished
     */
    void Finish();

    /** Every stream implementation has to call this in BeginAppend
     *  \param stream the stream object that is calling
     */
    void BeginAppendStream(const PdfStream& stream);

    /** Every stream implementation has to call this in EndAppend
     *  \param stream the stream object that is calling
     */
    void EndAppendStream(const PdfStream& stream);

    /**
     * Deletes all objects that are not references by other objects
     * besides the trailer (which references the root dictionary, which in
     * turn should reference all other objects).
     *
     * \param trailer trailer object of the PDF
     *
     * Warning this might be slow!
     */
    void CollectGarbage(PdfObject& trailer);

    /**
     * Set the object count so that the object described this reference
     * is contained in the object count.
     *
     * \param ref reference of newly added object
     */
    void TryIncrementObjectCount(const PdfReference& ref);

private:
    // Use deque as many insertions are here way faster than with using std::list
    // This is especially useful for PDFs like PDFReference17.pdf with
    // lots of free objects.
    typedef std::set<uint32_t>                       ObjectNumList;
    typedef std::set<PdfReference>                   ReferenceSet;
    typedef std::list<PdfReference*>                 ReferencePointers;
    typedef std::vector<ReferencePointers>           ReferencePointersList;
    typedef std::vector<Observer*> ObserverList;

private:
    // CHECK-ME: Should this function be public or stay private? Originally it was private
    /**
     *  Renumbers all objects according to there current position in the vector.
     *  All references remain intact.
     *  Warning! This function is _very_ calculation intensive.
     *
     *  \param trailer the trailer object
     *  \param notDelete a list of object which must not be deleted
     *  \param doGarbageCollection enable garbage collection, which deletes
     *         all objects that are not reachable from the trailer. This might be slow!
     *
     *  \see CollectGarbage
     */
    void RenumberObjects(PdfObject& trailer, ReferenceSet* notDelete = nullptr, bool doGarbageCollection = false);

    // CHECK-ME: Should this function be public or stay private? Originally it was private
    /** Get a set with all references of objects that the passed object
     *  depends on.
     *  \param obj the object to calculate all dependencies for
     *  \param list write the list of dependencies to this list
     *
     */
    void GetObjectDependencies(const PdfObject& obj, PdfReferenceList& list) const;

private:
    PdfIndirectObjectList(PdfDocument& document);
    PdfIndirectObjectList(PdfDocument& document, const PdfIndirectObjectList& rhs);

    PdfIndirectObjectList(const PdfIndirectObjectList&) = delete;
    PdfIndirectObjectList& operator=(const PdfIndirectObjectList&) = delete;

    /** Insert an object into this vector so that
     *  the vector remains sorted w.r.t.
     *  the ordering based on object and generation numbers
     *  m_ObjectCount will be increased for the object.
     *
     *  \param obj pointer to the object you want to insert
     */
    void PushObject(PdfObject* obj);

    /** Push an object with the givent reference. If one is existing, it will be replaced
     */
    void PushObject(const PdfReference& reference, PdfObject* obj);

    /** Mark a reference as unused so that it can be reused for new objects.
     *
     *  Add the object only if the generation is the allowed range
     *
     *  \param rReference the reference to reuse
     *  \returns true if the object was succesfully added
     *
     *  \see AddFreeObject
     */
    bool TryAddFreeObject(const PdfReference& reference);

    /** Mark a reference as unused so that it can be reused for new objects.
     *
     *  Add the object and increment the generation number. Add the object
     *  only if the generation is the allowed range
     *
     *  \param rReference the reference to reuse
     *  \returns the generation of the added free object
     *
     *  \see AddFreeObject
     */
    int32_t SafeAddFreeObject(const PdfReference& reference);

    /** Mark a reference as unused so that it can be reused for new objects.
     *  \param rReference the reference to reuse
     *
     *  \see GetCanReuseObjectNumbers
     */
    void AddFreeObject(const PdfReference& reference);

private:
    void addNewObject(PdfObject* obj);

    /**
     * \returns the next free object reference
     */
    PdfReference getNextFreeObject();

    int32_t tryAddFreeObject(uint32_t objnum, uint32_t gennum);

    /**
     * Create a list of all references that point to the object
     * for each object in this vector.
     * \param pList write all references to this list
     */
    void buildReferenceCountVector(ReferencePointersList& list);
    void insertReferencesIntoVector(const PdfObject& obj, ReferencePointersList& list);

    /** Assumes that the PdfIndirectObjectList is sorted
     */
    void insertOneReferenceIntoVector(const PdfObject& obj, ReferencePointersList& list);

    /** Delete all objects from the vector which do not have references to them selves
     *  \param pList must be a list created by BuildReferenceCountVector
     *  \param trailer must be the trailer object so that it is not deleted
     *  \param pNotDelete a list of object which must not be deleted
     *  \see BuildReferenceCountVector
     */
    void garbageCollection(ReferencePointersList& list, PdfObject& trailer, ReferenceSet* notDelete = nullptr);

public:
    /** Iterator pointing at the beginning of the vector
     *  \returns beginning iterator
     */
    iterator begin() const;

    /** Iterator pointing at the end of the vector
     *  \returns ending iterator
     */
    iterator end() const;

    size_t size() const;

public:
    /** \returns a pointer to a PdfDocument that is the
     *           parent of this vector.
     *           Might be nullptr if the vector has no parent.
     */
    inline PdfDocument& GetDocument() const { return *m_Document; }

    /**
     *  \returns whether can re-use free object numbers when creating new objects.
     */
    inline bool GetCanReuseObjectNumbers() const { return m_CanReuseObjectNumbers; }

    /** \returns a list of free references in this vector
     */
    inline const PdfReferenceList& GetFreeObjects() const { return m_FreeObjects; }

private:
    PdfDocument* m_Document;
    bool m_CanReuseObjectNumbers;
    ObjectList m_Objects;
    unsigned m_ObjectCount;
    PdfReferenceList m_FreeObjects;
    ObjectNumList m_UnavailableObjects;

    ObserverList m_observers;
    StreamFactory* m_StreamFactory;
};

};

#endif // PDF_INDIRECT_OBJECT_LIST_H
