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

#include <cassert>
#include <cstdlib>
#include <memory>
#include <mbstring.h>

#include "tinystr.h"

// TiXmlString constructor, based on a C string
TiXmlString::TiXmlString (const char* instring)
{
    unsigned newlen;
    wchar_t * newstring;

    if (!instring)
    {
        allocated = 0;
        wcstring = NULL;
		clength = 0;
        return;
    }

	newlen = _mbstrlen(instring) + 1;

	if (newlen < 0)
	{
		allocated = 0;
		wcstring = NULL;
		clength = 0;
		return;
	}
    newstring = new wchar_t [newlen];
	mbstowcs(newstring, instring, newlen);
    allocated = newlen;
    wcstring = newstring;
	clength = newlen - 1;
}

// TiXmlString constructor, based on a Unicode string
TiXmlString::TiXmlString (const wchar_t* instring)
{
    unsigned newlen;
    wchar_t * newstring;

    if (!instring)
    {
        allocated = 0;
        wcstring = NULL;
		clength = 0;
        return;
    }
    newlen = wcslen (instring) + 1;
    newstring = new wchar_t [newlen];
    wcscpy (newstring, instring);
    allocated = newlen;
    wcstring = newstring;
	clength = newlen - 1;
}

// TiXmlString copy constructor
TiXmlString::TiXmlString (const TiXmlString& copy)
{
    //unsigned newlen;
    wchar_t * newstring;

	// Prevent copy to self!
	if ( &copy == this )
		return;

    if (! copy . allocated)
    {
        allocated = 0;
        wcstring = NULL;
		clength = 0;
        return;
    }
    //newlen = wcslen (copy . wcstring) + 1;
	newstring = new wchar_t [copy.clength + 1];
    wcscpy (newstring, copy . wcstring);
	allocated = copy.clength + 1;
    wcstring = newstring;
	clength = copy.clength;
}

// TiXmlString = operator. Safe when assign own content
void TiXmlString ::operator = (const char * content)
{
    unsigned newlen;
    wchar_t * newstring;

    if (! content)
    {
        empty_it ();
        return;
    }
	newlen = _mbstrlen(content) + 1;
    newstring = new wchar_t [newlen];
    mbstowcs (newstring, content, newlen);
    empty_it ();
    allocated = newlen;
    wcstring = newstring;
	clength = newlen - 1;
}

// TiXmlString = operator. Safe when assign own content
void TiXmlString ::operator = (const wchar_t * content)
{
    unsigned newlen;
	wchar_t * newstring;

    if (! content)
    {
        empty_it ();
        return;
    }
    newlen = wcslen (content) + 1;
    newstring = new wchar_t [newlen];
    wcscpy (newstring, content);
    empty_it ();
    allocated = newlen;
    wcstring = newstring;
	clength = newlen - 1;
}

// = operator. Safe when assign own content
void TiXmlString ::operator = (const TiXmlString & copy)
{
    //unsigned newlen;
    wchar_t * newstring;

    if (! copy . clength)
    {
        empty_it ();
        return;
    }
    //newlen = copy . length () + 1;
	newstring = new wchar_t [copy.clength + 1];
    wcscpy (newstring, copy . wc_str ());
    empty_it ();
    allocated = copy.clength + 1;
    wcstring = newstring;
	clength = copy.clength;
}


//// Checks if a TiXmlString contains only whitespace (same rules as isspace)
//bool TiXmlString::isblank () const
//{
//    char * lookup;
//    for (lookup = cstring; * lookup; lookup++)
//        if (! isspace (* lookup))
//            return false;
//    return true;
//}

// append a const char * to an existing TiXmlString
/*
void TiXmlString::append( const char* str, int len )
{
    char * new_string;
    unsigned new_alloc, new_size;

    new_size = length () + len + 1;
    // check if we need to expand
    if (new_size > allocated)
    {
        // compute new size
        new_alloc = assign_new_size (new_size);

        // allocate new buffer
        new_string = new char [new_alloc];        
        new_string [0] = 0;

        // copy the previous allocated buffer into this one
        if (allocated && cstring)
            strcpy (new_string, cstring);

        // append the suffix. It does exist, otherwize we wouldn't be expanding 
        strncat (new_string, str, len);

        // return previsously allocated buffer if any
        if (allocated && cstring)
            delete [] cstring;

        // update member variables
        cstring = new_string;
        allocated = new_alloc;
    }
    else
        // we know we can safely append the new string
        strncat (cstring, str, len);
}
*/

// append a const wchar_t * to an existing TiXmlString
void TiXmlString::append( const wchar_t* str, int len )
{
    if (len == 1)
	{
		append(str[0]);
		return;
	}
	wchar_t * new_string;
    unsigned new_alloc, new_size;

    new_size = clength + len + 2;
    // check if we need to expand
    if (new_size > allocated)
    {
        // compute new size
        new_alloc = assign_new_size (new_size);

        // allocate new buffer
        new_string = new wchar_t [new_alloc];        
        new_string [0] = 0;

        // copy the previous allocated buffer into this one
        if (allocated && wcstring)
            //wcscpy (new_string, wcstring);
			memcpy((void*)new_string, (void*)wcstring, clength * sizeof(wchar_t));

        // append the suffix. It does exist, otherwize we wouldn't be expanding 
        //wcsncat (new_string, str, len);
		memcpy((void*)(new_string + clength), (void*)str, len * sizeof(wchar_t));
		clength += len;

        // return previsously allocated buffer if any
        if (allocated && wcstring)
            delete [] wcstring;

        // update member variables
        wcstring = new_string;
        allocated = new_alloc;
    }
    else
	{
		// we know we can safely append the new string
        //wcsncat (wcstring, str, len);
		memcpy((void*)(wcstring + clength), (void*)str, len * sizeof(wchar_t));
		clength += len;
	}
	if (wcstring[clength] != 0)
		wcstring[clength] = 0;
}


// append a const wchar_t * to an existing TiXmlString
void TiXmlString::append( const wchar_t * suffix )
{
    wchar_t * new_string;
    unsigned new_alloc, new_size;
	unsigned len = wcslen (suffix);
    new_size = clength + len + 2;
    // check if we need to expand
    if (new_size > allocated)
    {
        // compute new size
        new_alloc = assign_new_size (new_size);

        // allocate new buffer
        new_string = new wchar_t [new_alloc];        
        new_string [0] = 0;

        // copy the previous allocated buffer into this one
        if (allocated && wcstring)
            //wcscpy (new_string, wcstring);
			memcpy((void*)new_string, (void*)wcstring, clength * sizeof(wchar_t));

        // append the suffix. It does exist, otherwize we wouldn't be expanding 
        //wcscat (new_string, suffix);
		memcpy((void*)(new_string + clength), (void*)suffix, (len + 1) * sizeof(wchar_t));
		clength += len;

        // return previsously allocated buffer if any
        if (allocated && wcstring)
            delete [] wcstring;

        // update member variables
        wcstring = new_string;
        allocated = new_alloc;
    }
    else
	{
		// we know we can safely append the new string
        //wcscat (wcstring, suffix);
		memcpy((void*)(wcstring + clength), (void*)suffix, (len + 1) * sizeof(wchar_t));
		clength += len;
	}
	if (wcstring[clength] != 0)
		wcstring[clength] = 0;
}

// Check for TiXmlString equuivalence
//bool TiXmlString::operator == (const TiXmlString & compare) const
//{
//    return (! strcmp (c_str (), compare . c_str ()));
//}

unsigned TiXmlString::length () const
{
    if (allocated)
        return clength;//wcslen (wcstring);
    return 0;
}


/*
unsigned TiXmlString::find (char tofind, unsigned offset) const
{
    char * lookup;

    if (offset >= length ())
        return (unsigned) notfound;
    for (lookup = cstring + offset; * lookup; lookup++)
        if (* lookup == tofind)
            return lookup - cstring;
    return (unsigned) notfound;
}
*/
unsigned TiXmlString::find (wchar_t tofind, unsigned offset) const
{
	wchar_t * lookup;

	if (offset >= clength )
		return (unsigned) notfound;
	for (lookup = wcstring + offset; * lookup; lookup++)
		if (* lookup == tofind)
			return lookup - wcstring;
	return (unsigned) notfound;
}


bool TiXmlString::operator == (const TiXmlString & compare) const
{
	if ( allocated && compare.allocated )
	{
		assert( wcstring );
		assert( compare.wcstring );
		if (clength != compare.clength)
			return false;
		return ( _wcsicmp( wcstring, compare.wcstring ) == 0 );
 	}
	return false;
}


bool TiXmlString::operator < (const TiXmlString & compare) const
{
	if ( allocated && compare.allocated )
	{
		assert( wcstring );
		assert( compare.wcstring );
		return ( _wcsicmp( wcstring, compare.wcstring ) > 0 );
 	}
	return false;
}


bool TiXmlString::operator > (const TiXmlString & compare) const
{
	if ( allocated && compare.allocated )
	{
		assert( wcstring );
		assert( compare.wcstring );
		return ( _wcsicmp( wcstring, compare.wcstring ) < 0 );
 	}
	return false;
}


#endif	// TIXML_USE_STL
