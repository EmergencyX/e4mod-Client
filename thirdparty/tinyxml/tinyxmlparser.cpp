/*
www.sourceforge.net/projects/tinyxml
Original code (2.0 and earlier )copyright (c) 2000-2002 Lee Thomason (www.grinninglizard.com)

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

#include "tinyxml.h"
#include <ctype.h>

//#define DEBUG_PARSER

// Note tha "PutString" hardcodes the same list. This
// is less flexible than it appears. Changing the entries
// or order will break putstring.	
TiXmlBase::Entity TiXmlBase::entity[ NUM_ENTITY ] = 
{
	{ L"&amp;",  5, L'&' },
	{ L"&lt;",   4, L'<' },
	{ L"&gt;",   4, L'>' },
	{ L"&quot;", 6, L'\"' },
	{ L"&apos;", 6, L'\'' }
};


const wchar_t* TiXmlBase::SkipWhiteSpace( const wchar_t* p )
{
	if ( !p || !*p )
	{
		return 0;
	}
	while ( p && *p )
	{
		if ( iswspace( *p ) || *p == '\n' || *p =='\r' )		// Still using old rules for white space.
			++p;
		else
			break;
	}

	return p;
}

#ifdef TIXML_USE_STL
/*static*/ bool TiXmlBase::StreamWhiteSpace( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	for( ;; )
	{
		if ( !in->good() ) return false;

		int c = in->peek();
		if ( !IsWhiteSpace( c ) )
			return true;
		*tag += in->get();
	}
}

/*static*/ bool TiXmlBase::StreamTo( TIXML_ISTREAM * in, int character, TIXML_STRING * tag )
{
	while ( in->good() )
	{
		int c = in->peek();
		if ( c == character )
			return true;

		in->get();
		*tag += c;
	}
	return false;
}
#endif

const wchar_t* TiXmlBase::ReadName( const wchar_t* p, TIXML_STRING * name )
{
	*name = "";
	assert( p );

	// Names start with letters or underscores.
	// After that, they can be letters, underscores, numbers,
	// hyphens, or colons. (Colons are valid ony for namespaces,
	// but tinyxml can't tell namespaces from names.)
	if (    p && *p 
		 && ( iswalpha( (unsigned char) *p ) || *p == '_' ) )
	{
		while(		p && *p
				&&	(		iswalnum( /*(unsigned char )*/ *p ) 
						 || *p == '_'
						 || *p == '-'
						 || *p == ':' ) )
		{
			(*name) += *p;
			++p;
		}
		return p;
	}
	return 0;
}

const wchar_t* TiXmlBase::GetEntity( const wchar_t* p, wchar_t* value )
{
	// Presume an entity, and pull it out.
    TIXML_STRING ent;
	int i;

	// Ignore the &#x entities.
	if (    wcsncmp( L"&#x", p, 3 ) == 0 
	     && *(p+3) 
		 && *(p+4) )
	{
		*value = 0;
		
		if ( iswalpha( *(p+3) ) ) *value += ( towlower( *(p+3) ) - 'a' + 10 ) * 16;
		else				     *value += ( *(p+3) - '0' ) * 16;

		if ( iswalpha( *(p+4) ) ) *value += ( towlower( *(p+4) ) - 'a' + 10 );
		else				     *value += ( *(p+4) - '0' );

		return p+6;
	}

	// Now try to match it.
	for( i=0; i<NUM_ENTITY; ++i )
	{
		if ( wcsncmp( entity[i].str, p, entity[i].strLength ) == 0 )
		{
			assert( wcslen( entity[i].str ) == entity[i].strLength );
			*value = entity[i].chr;
			return ( p + entity[i].strLength );
		}
	}

	// So it wasn't an entity, its unrecognized, or something like that.
	*value = *p;	// Don't put back the last one, since we return it!
	return p+1;
}


bool TiXmlBase::StringEqual( const wchar_t* p,
							 const wchar_t* tag,
							 bool ignoreCase )
{
	assert( p );
	if ( !p || !*p )
	{
		assert( 0 );
		return false;
	}

    if ( towlower( *p ) == towlower( *tag ) )
	{
		const wchar_t* q = p;

		if (ignoreCase)
		{
			while ( *q && *tag && *q == *tag )
			{
				++q;
				++tag;
			}

			if ( *tag == 0 )		// Have we found the end of the tag, and everything equal?
			{
				return true;
			}
		}
		else
		{
			while ( *q && *tag && towlower( *q ) == towlower( *tag ) )
			{
				++q;
				++tag;
			}

			if ( *tag == 0 )
			{
				return true;
			}
		}
	}
	return false;
}

const wchar_t* TiXmlBase::ReadText(	const wchar_t* p, 
									TIXML_STRING * text, 
									bool trimWhiteSpace, 
									const wchar_t* endTag, 
									bool caseInsensitive )
{
    *text = L"";
	if (    !trimWhiteSpace			// certain tags always keep whitespace
		 || !condenseWhiteSpace )	// if true, whitespace is always kept
	{
		// Keep all the white space.
		while (	   p && *p
				&& !StringEqual( p, endTag, caseInsensitive )
			  )
		{
			wchar_t c;
			p = GetChar( p, &c );
            (* text) += c;
		}
	}
	else
	{
		bool whitespace = false;

		// Remove leading white space:
		p = SkipWhiteSpace( p );
		while (	   p && *p
				&& !StringEqual( p, endTag, caseInsensitive ) )
		{
			if ( *p == L'\r' || *p == L'\n' )
			{
				whitespace = true;
				++p;
			}
			else if ( iswspace( *p ) )
			{
				whitespace = true;
				++p;
			}
			else
			{
				// If we've found whitespace, add it before the
				// new character. Any whitespace just becomes a space.
				if ( whitespace )
				{
               (* text) += L' ';
					whitespace = false;
				}
				wchar_t c;
				p = GetChar( p, &c );
            (* text) += c;
			}
		}
	}
	return p + wcslen( endTag );
}

#ifdef TIXML_USE_STL

void TiXmlDocument::StreamIn( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	// The basic issue with a document is that we don't know what we're
	// streaming. Read something presumed to be a tag (and hope), then
	// identify it, and call the appropriate stream method on the tag.
	//
	// This "pre-streaming" will never read the closing ">" so the
	// sub-tag can orient itself.

	if ( !StreamTo( in, '<', tag ) ) 
	{
		SetError( TIXML_ERROR_PARSING_EMPTY );
		return;
	}

	while ( in->good() )
	{
		int tagIndex = tag->length();
		while ( in->good() && in->peek() != '>' )
		{
			int c = in->get();
			(*tag) += (wchar_t) c;
		}

		if ( in->good() )
		{
			// We now have something we presume to be a node of 
			// some sort. Identify it, and call the node to
			// continue streaming.
			TiXmlNode* node = Identify( tag->wc_str() + tagIndex );

			if ( node )
			{
				node->StreamIn( in, tag );
				bool isElement = node->ToElement() != 0;
				delete node;
				node = 0;

				// If this is the root element, we're done. Parsing will be
				// done by the >> operator.
				if ( isElement )
				{
					return;
				}
			}
			else
			{
				SetError( TIXML_ERROR );
				return;
			}
		}
	}
	// We should have returned sooner.
	SetError( TIXML_ERROR );
}

#endif

const wchar_t* TiXmlDocument::Parse( const wchar_t* p )
{
	// Parse away, at the document level. Since a document
	// contains nothing but other tags, most of what happens
	// here is skipping white space.
	//
	// In this variant (as opposed to stream and Parse) we
	// read everything we can.


	if ( !p || !*p )
	{
		SetError( TIXML_ERROR_DOCUMENT_EMPTY );
		return false;
	}

    p = SkipWhiteSpace( p );
	if ( !p )
	{
		SetError( TIXML_ERROR_DOCUMENT_EMPTY );
		return false;
	}

	while ( p && *p )
	{
		TiXmlNode* node = Identify( p );
		if ( node )
		{
			p = node->Parse( p );
			LinkEndChild( node );
		}
		else
		{
			break;
		}
		p = SkipWhiteSpace( p );
	}
	// All is well.
	return p;
}


TiXmlNode* TiXmlNode::Identify( const wchar_t* p )
{
	TiXmlNode* returnNode = 0;

	p = SkipWhiteSpace( p );
	if( !p || !*p || *p != L'<' )
	{
		return 0;
	}

	TiXmlDocument* doc = GetDocument();
	p = SkipWhiteSpace( p );

	if ( !p || !*p )
	{
		return 0;
	}

	// What is this thing? 
	// - Elements start with a letter or underscore, but xml is reserved.
	// - Comments: <!--
	// - Decleration: <?xml
	// - Everthing else is unknown to tinyxml.
	//

	const wchar_t* xmlHeader = { L"<?xml" };
	const wchar_t* commentHeader = { L"<!--" };

	if ( StringEqual( p, xmlHeader, true ) )
	{
		#ifdef DEBUG_PARSER
			TIXML_LOG( "XML parsing Declaration\n" );
		#endif
		returnNode = new TiXmlDeclaration();
	}
	else if (    iswalpha( *(p+1) )
			  || *(p+1) == '_' )
	{
		#ifdef DEBUG_PARSER
			TIXML_LOG( "XML parsing Element\n" );
		#endif
		returnNode = new TiXmlElement( L"" );
	}
	else if ( StringEqual( p, commentHeader, false ) )
	{
		#ifdef DEBUG_PARSER
			TIXML_LOG( "XML parsing Comment\n" );
		#endif
		returnNode = new TiXmlComment();
	}
	else
	{
		#ifdef DEBUG_PARSER
			TIXML_LOG( "XML parsing Unknown\n" );
		#endif
		returnNode = new TiXmlUnknown();
	}

	if ( returnNode )
	{
		// Set the parent, so it can report errors
		returnNode->parent = this;
		//p = returnNode->Parse( p );
	}
	else
	{
		if ( doc )
			doc->SetError( TIXML_ERROR_OUT_OF_MEMORY );
	}
	return returnNode;
}

#ifdef TIXML_USE_STL

void TiXmlElement::StreamIn (TIXML_ISTREAM * in, TIXML_STRING * tag)
{
	// We're called with some amount of pre-parsing. That is, some of "this"
	// element is in "tag". Go ahead and stream to the closing ">"
	while( in->good() )
	{
		int c = in->get();
		(*tag) += (wchar_t) c ;
		
		if ( c == '>' )
			break;
	}

	if ( tag->length() < 3 ) return;

	// Okay...if we are a "/>" tag, then we're done. We've read a complete tag.
	// If not, identify and stream.

	if (    tag->at( tag->length() - 1 ) == '>' 
		 && tag->at( tag->length() - 2 ) == '/' )
	{
		// All good!
		return;
	}
	else if ( tag->at( tag->length() - 1 ) == '>' )
	{
		// There is more. Could be:
		//		text
		//		closing tag
		//		another node.
		for ( ;; )
		{
			StreamWhiteSpace( in, tag );

			// Do we have text?
			if ( in->peek() != '<' )
			{
				// Yep, text.
				TiXmlText text( "" );
				text.StreamIn( in, tag );

				// What follows text is a closing tag or another node.
				// Go around again and figure it out.
				continue;
			}

			// We now have either a closing tag...or another node.
			// We should be at a "<", regardless.
			if ( !in->good() ) return;
			assert( in->peek() == '<' );
			int tagIndex = tag->length();

			bool closingTag = false;
			bool firstCharFound = false;

			for( ;; )
			{
				if ( !in->good() )
					return;

				int c = in->peek();
				
				if ( c == '>' )
					break;

				*tag += c;
				in->get();

				if ( !firstCharFound && c != '<' && !IsWhiteSpace( c ) )
				{
					firstCharFound = true;
					if ( c == '/' )
						closingTag = true;
				}
			}
			// If it was a closing tag, then read in the closing '>' to clean up the input stream.
			// If it was not, the streaming will be done by the tag.
			if ( closingTag )
			{
				int c = in->get();
				assert( c == '>' );
				*tag += c;

				// We are done, once we've found our closing tag.
				return;
			}
			else
			{
				// If not a closing tag, id it, and stream.
				const wchar_t* tagloc = tag->wc_str() + tagIndex;
				TiXmlNode* node = Identify( tagloc );
				if ( !node )
					return;
				node->StreamIn( in, tag );
				delete node;
				node = 0;

				// No return: go around from the beginning: text, closing tag, or node.
			}
		}
	}
}
#endif

const wchar_t* TiXmlElement::Parse( const wchar_t* p )
{
	p = SkipWhiteSpace( p );
	TiXmlDocument* document = GetDocument();

	if ( !p || !*p || *p != '<' )
	{
		if ( document ) document->SetError( TIXML_ERROR_PARSING_ELEMENT );
		return false;
	}

	p = SkipWhiteSpace( p+1 );

	// Read the name.
    p = ReadName( p, &value );
	if ( !p || !*p )
	{
		if ( document )	document->SetError( TIXML_ERROR_FAILED_TO_READ_ELEMENT_NAME );
		return false;
	}

    TIXML_STRING endTag ("</");
	endTag += value;
	endTag += ">";

	// Check for and read attributes. Also look for an empty
	// tag or an end tag.
	while ( p && *p )
	{
		p = SkipWhiteSpace( p );
		if ( !p || !*p )
		{
			if ( document ) document->SetError( TIXML_ERROR_READING_ATTRIBUTES );
			return 0;
		}
		if ( *p == '/' )
		{
			++p;
			// Empty tag.
			if ( *p  != '>' )
			{
				if ( document ) document->SetError( TIXML_ERROR_PARSING_EMPTY );		
				return 0;
			}
			return (p+1);
		}
		else if ( *p == '>' )
		{
			// Done with attributes (if there were any.)
			// Read the value -- which can include other
			// elements -- read the end tag, and return.
			++p;
			p = ReadValue( p );		// Note this is an Element method, and will set the error if one happens.
			if ( !p || !*p )
				return 0;

			// We should find the end tag now
			if ( StringEqual( p, endTag.wc_str(), false ) )
			{
				p += endTag.length();
				return p;
			}
			else
			{
				if ( document ) document->SetError( TIXML_ERROR_READING_END_TAG );
				return 0;
			}
		}
		else
		{
			// Try to read an element:
			TiXmlAttribute attrib;
			attrib.SetDocument( document );
			p = attrib.Parse( p );

			if ( !p || !*p )
			{
				if ( document ) document->SetError( TIXML_ERROR_PARSING_ELEMENT );
				return 0;
			}
			SetAttribute( attrib.Name(), attrib.Value() );
		}
	}
	return p;
}


const wchar_t* TiXmlElement::ReadValue( const wchar_t* p )
{
	TiXmlDocument* document = GetDocument();

	// Read in text and elements in any order.
	p = SkipWhiteSpace( p );
	while ( p && *p )
	{
		if ( *p != '<' )
		{
			// Take what we have, make a text element.
			TiXmlText* textNode = new TiXmlText( L"" );

			if ( !textNode )
			{
				if ( document ) document->SetError( TIXML_ERROR_OUT_OF_MEMORY );
				    return 0;
			}

			p = textNode->Parse( p );

			if ( !textNode->Blank() )
				LinkEndChild( textNode );
			else
				delete textNode;
		} 
		else 
		{
			// We hit a '<'
			// Have we hit a new element or an end tag?
			if ( StringEqual( p, L"</", false ) )
			{
				return p;
			}
			else
			{
				TiXmlNode* node = Identify( p );
				if ( node )
				{
					p = node->Parse( p );
					LinkEndChild( node );
				}				
				else
				{
					return 0;
				}
			}
		}
		p = SkipWhiteSpace( p );
	}

	if ( !p )
	{
		if ( document ) document->SetError( TIXML_ERROR_READING_ELEMENT_VALUE );
	}	
	return p;
}


#ifdef TIXML_USE_STL
void TiXmlUnknown::StreamIn( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	while ( in->good() )
	{
		int c = in->get();	
		(*tag) += c;

		if ( c == '>' )
		{
			// All is well.
			return;		
		}
	}
}
#endif


const wchar_t* TiXmlUnknown::Parse( const wchar_t* p )
{
	TiXmlDocument* document = GetDocument();
	p = SkipWhiteSpace( p );
	if ( !p || !*p || *p != '<' )
	{
		if ( document ) document->SetError( TIXML_ERROR_PARSING_UNKNOWN );
		return 0;
	}
	++p;
    value = L"";

	while ( p && *p && *p != '>' )
	{
		value += *p;
		++p;
	}

	if ( !p )
	{
		if ( document )	document->SetError( TIXML_ERROR_PARSING_UNKNOWN );
	}
	if ( *p == '>' )
		return p+1;
	return p;
}

#ifdef TIXML_USE_STL
void TiXmlComment::StreamIn( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	while ( in->good() )
	{
		int c = in->get();	
		(*tag) += c;

		if ( c == '>' 
			 && tag->at( tag->length() - 2 ) == '-'
			 && tag->at( tag->length() - 3 ) == '-' )
		{
			// All is well.
			return;		
		}
	}
}
#endif


const wchar_t* TiXmlComment::Parse( const wchar_t* p )
{
	TiXmlDocument* document = GetDocument();
	value = "";

	p = SkipWhiteSpace( p );
	const wchar_t* startTag = L"<!--";
	const wchar_t* endTag   = L"-->";

	if ( !StringEqual( p, startTag, false ) )
	{
		document->SetError( TIXML_ERROR_PARSING_COMMENT );
		return 0;
	}
	p += wcslen( startTag );
	p = ReadText( p, &value, false, endTag, false );
	return p;
}


const wchar_t* TiXmlAttribute::Parse( const wchar_t* p )
{
	p = SkipWhiteSpace( p );
	if ( !p || !*p ) return 0;

	// Read the name, the '=' and the value.
	p = ReadName( p, &name );
	if ( !p || !*p )
	{
		if ( document ) document->SetError( TIXML_ERROR_READING_ATTRIBUTES );
		return 0;
	}
	p = SkipWhiteSpace( p );
	if ( !p || !*p || *p != '=' )
	{
		if ( document ) document->SetError( TIXML_ERROR_READING_ATTRIBUTES );
		return 0;
	}

	++p;	// skip '='
	p = SkipWhiteSpace( p );
	if ( !p || !*p )
	{
		if ( document ) document->SetError( TIXML_ERROR_READING_ATTRIBUTES );
		return 0;
	}
	
	const wchar_t* end;

	if ( *p == '\'' )
	{
		++p;
		end = L"\'";
		p = ReadText( p, &value, false, end, false );
	}
	else if ( *p == '"' )
	{
		++p;
		end = L"\"";
		p = ReadText( p, &value, false, end, false );
	}
	else
	{
		// All attribute values should be in single or double quotes.
		// But this is such a common error that the parser will try
		// its best, even without them.
		value = L"";
		while (    p && *p										// existence
				&& !iswspace( *p ) && *p != '\n' && *p != '\r'	// whitespace
				&& *p != '/' && *p != '>' )						// tag end
		{
			value += *p;
			++p;
		}
	}
	return p;
}

#ifdef TIXML_USE_STL
void TiXmlText::StreamIn( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	while ( in->good() )
	{
		int c = in->peek();	
		if ( c == '<' )
			return;

		(*tag) += c;
		in->get();
	}
}
#endif

const wchar_t* TiXmlText::Parse( const wchar_t* p )
{
	value = L"";

	//TiXmlDocument* doc = GetDocument();
	bool ignoreWhite = true;
//	if ( doc && !doc->IgnoreWhiteSpace() ) ignoreWhite = false;

	const wchar_t* end = L"<";
	p = ReadText( p, &value, ignoreWhite, end, false );
	if ( p )
		return p-1;	// don't truncate the '<'
	return 0;
}

#ifdef TIXML_USE_STL
void TiXmlDeclaration::StreamIn( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	while ( in->good() )
	{
		int c = in->get();
		(*tag) += c;

		if ( c == '>' )
		{
			// All is well.
			return;
		}
	}
}
#endif

const wchar_t* TiXmlDeclaration::Parse( const wchar_t* p )
{
	p = SkipWhiteSpace( p );
	// Find the beginning, find the end, and look for
	// the stuff in-between.
	TiXmlDocument* document = GetDocument();
	if ( !p || !*p || !StringEqual( p, L"<?xml", true ) )
	{
		if ( document ) document->SetError( TIXML_ERROR_PARSING_DECLARATION );
		return 0;
	}

	p += 5;
//	const char* start = p+5;
//	const char* end  = strstr( start, "?>" );

	version = L"";
	encoding = L"";
	standalone = L"";

	while ( p && *p )
	{
		if ( *p == '>' )
		{
			++p;
			return p;
		}

		p = SkipWhiteSpace( p );
		if ( StringEqual( p, L"version", true ) )
		{
//			p += 7;
			TiXmlAttribute attrib;
			p = attrib.Parse( p );		
			version = attrib.Value();
		}
		else if ( StringEqual( p, L"encoding", true ) )
		{
//			p += 8;
			TiXmlAttribute attrib;
			p = attrib.Parse( p );		
			encoding = attrib.Value();
		}
		else if ( StringEqual( p, L"standalone", true ) )
		{
//			p += 10;
			TiXmlAttribute attrib;
			p = attrib.Parse( p );		
			standalone = attrib.Value();
		}
		else
		{
			// Read over whatever it is.
			while( p && *p && *p != '>' && !iswspace( *p ) )
				++p;
		}
	}
	return 0;
}

bool TiXmlText::Blank() const
{
	for ( unsigned i=0; i<value.length(); i++ )
		if ( !iswspace( value[i] ) )
			return false;
	return true;
}

