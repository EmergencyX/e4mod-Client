/*
www.sourceforge.net/projects/tinyxml
Original file by Yves Berquin.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#ifndef TIXML_USE_STL

#ifndef TIXML_STRING_INCLUDED
#define TIXML_STRING_INCLUDED

#pragma warning( disable : 4514 )

/*
   TiXmlString is an emulation of the std::string template.
   Its purpose is to allow compiling TinyXML on compilers with no or poor STL support.
   Only the member functions relevant to the TinyXML project have been implemented.
   The buffer allocation is made by a simplistic power of 2 like mechanism : if we increase
   a string and there's no more room, we allocate a buffer twice as big as we need.
*/
class TiXmlString
{
  public :
    // TiXmlString constructor, based on a string
    TiXmlString (const char * instring);
	TiXmlString (const wchar_t * instring);

    // TiXmlString empty constructor
    TiXmlString ()
    {
        allocated = 0;
		clength = 0;
        wcstring = NULL;
    }

    // TiXmlString copy constructor
    TiXmlString (const TiXmlString& copy);

    // TiXmlString destructor
    ~ TiXmlString ()
    {
        empty_it ();
    }
	
    // Convert a TiXmlString into a classical char *
	/*
    const char * c_str () const
    {
        if (allocated)
            return cstring;
        return "";
    }
	*/
	const wchar_t * wc_str () const
	{
		if (allocated)
			return wcstring;
		return L"";
	}

    // Return the length of a TiXmlString
    unsigned length () const;

    // TiXmlString = operator
    void operator = (const char * content);
	void operator = (const wchar_t * content);

    // = operator
    void operator = (const TiXmlString & copy);

    // += operator. Maps to append
    TiXmlString& operator += (const char * suffix)
    {
        append (suffix);
		return *this;
    }
	TiXmlString& operator += (const wchar_t * suffix)
	{
		append (suffix);
		return *this;
	}

    // += operator. Maps to append
    TiXmlString& operator += (char single)
    {
        append (single);
		return *this;
    }
	TiXmlString& operator += (wchar_t single)
	{
		append (single);
		return *this;
	}

    // += operator. Maps to append
    TiXmlString& operator += (TiXmlString & suffix)
    {
        append (suffix);
		return *this;
    }
    bool operator == (const TiXmlString & compare) const;
    bool operator < (const TiXmlString & compare) const;
    bool operator > (const TiXmlString & compare) const;

    // Checks if a TiXmlString is empty
    bool empty () const
    {
        return clength ? false : true;
    }

    // Checks if a TiXmlString contains only whitespace (same rules as isspace)
	// Not actually used in tinyxml. Conflicts with a C macro, "isblank",
	// which is a problem. Commenting out. -lee
//    bool isblank () const;

    // single char extraction
	/*
    const char& at (unsigned index) const
    {
        assert( index < length ());
        return cstring [index];
    }
	*/
	const wchar_t& at (unsigned index) const
	{
		assert( index < clength );
		return wcstring [index];
	}

    // find a char in a string. Return TiXmlString::notfound if not found
	/*
    unsigned find (char lookup) const
    {
        return find (lookup, 0);
    }
	*/
	unsigned find (wchar_t lookup) const
	{
		return find (lookup, 0);
	}

    // find a char in a string from an offset. Return TiXmlString::notfound if not found
	/*
    unsigned find (char tofind, unsigned offset) const;
	*/
	unsigned find (wchar_t tofind, unsigned offset) const;

    /*	Function to reserve a big amount of data when we know we'll need it. Be aware that this
		function clears the content of the TiXmlString if any exists.
    */
    void reserve (unsigned size)
    {
        empty_it ();
        if (size)
        {
            allocated = size;
            wcstring = new wchar_t [size];
            wcstring [0] = 0;
        }
    }

    // [] operator 
	/*
    char& operator [] (unsigned index) const
    {
        assert( index < length ());
        return cstring [index];
    }
	*/
	wchar_t& operator [] (unsigned index) const
	{
		assert( index < clength );
		return wcstring [index];
	}

    // Error value for find primitive 
    enum {	notfound = 0xffffffff,
            npos = notfound };

	/*
    void append (const char *str, int len );
	*/
	void append (const wchar_t *str, int len );

  protected :

    // The base string
    wchar_t * wcstring;
    // Number of chars allocated
    unsigned allocated;
	// Length of the String (number of chars)
	unsigned clength;

    // New size computation. It is simplistic right now : it returns twice the amount
    // we need
    unsigned assign_new_size (unsigned minimum_to_allocate)
    {
        return minimum_to_allocate * 2;
    }

    // Internal function that clears the content of a TiXmlString
    void empty_it ()
    {
        if (wcstring)
            delete [] wcstring;
        wcstring = NULL;
        allocated = 0;
		clength = 0;
    }

    void append (const wchar_t *suffix );

    // append function for another TiXmlString
    void append (const TiXmlString & suffix)
    {
        //append (suffix . wc_str ());
		if (clength + suffix.clength + 1 > allocated)
		{
			wchar_t * new_string;
			unsigned new_alloc, new_len;
			// compute new size
			new_alloc = assign_new_size (clength + suffix.clength);

			// allocate new buffer
			new_string = new wchar_t [new_alloc];        
			new_string [0] = 0;
			new_len = clength;

			// copy the previous allocated buffer into this one
			if (allocated && wcstring)
				//wcscpy (new_string, wcstring);
				memcpy((void*)new_string, (void*)wcstring, clength * sizeof(wchar_t));
			empty_it();
			wcstring = new_string;
			allocated = new_alloc;
			clength = new_len;
		}
		memcpy((void*)(wcstring + clength), (void*)suffix.wcstring, (suffix.clength + 1) * sizeof(wchar_t));
		clength += suffix.clength;
    }

    // append for a single char. This could be improved a lot if needed
    void append (wchar_t single)
    {
        //wchar_t smallstr [2];
        //smallstr [0] = single;
        //smallstr [1] = 0;
        //append (smallstr);
		if (wcstring)
		{
			wchar_t * new_string;
			unsigned new_alloc, new_len;
			if (clength + 1 >= allocated)
			{
				// compute new size
				new_alloc = assign_new_size (allocated);

				// allocate new buffer
				new_string = new wchar_t [new_alloc];        
				new_string [0] = 0;
				new_len = clength;

				// copy the previous allocated buffer into this one
				if (allocated && wcstring)
					//wcscpy (new_string, wcstring);
					memcpy((void*)new_string, (void*)wcstring, clength * sizeof(wchar_t));
				empty_it();
				wcstring = new_string;
				allocated = new_alloc;
				clength = new_len;
			}
			wcstring[clength++] = single;
			wcstring[clength] = 0;
		}
		else
		{
			wcstring = new wchar_t [2];
			wcstring[0] = single;
			wcstring[1] = 0;
			allocated = 2;
			clength = 1;
		}
    }

} ;

/* 
   TiXmlOutStream is an emulation of std::ostream. It is based on TiXmlString.
   Only the operators that we need for TinyXML have been developped.
*/
class TiXmlOutStream : public TiXmlString
{
public :
    TiXmlOutStream () : TiXmlString () {}

    // TiXmlOutStream << operator. Maps to TiXmlString::append
    TiXmlOutStream & operator << (const wchar_t * in)
    {
        append (in);
        return (* this);
    }

    // TiXmlOutStream << operator. Maps to TiXmlString::append
    TiXmlOutStream & operator << (const TiXmlString & in)
    {
        append (in . wc_str ());
        return (* this);
    }
} ;

#endif	// TIXML_STRING_INCLUDED
#endif	// TIXML_USE_STL