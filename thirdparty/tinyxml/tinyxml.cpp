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

#include <ctype.h>
#include "tinyxml.h"

bool TiXmlBase::condenseWhiteSpace = true;

void TiXmlBase::PutString( const TIXML_STRING& str, TIXML_OSTREAM* stream )
{
	TIXML_STRING buffer;
	PutString( str, &buffer );
	(*stream) << buffer;
}

void TiXmlBase::PutString( const TIXML_STRING& str, TIXML_STRING* outString )
{
	int i=0;

	while( i<(int)str.length() )
	{
		int c = str[i];

		if (    c == '&' 
		     && i < ( (int)str.length() - 2 )
			 && str[i+1] == '#'
			 && str[i+2] == 'x' )
		{
			// Hexadecimal character reference.
			// Pass through unchanged.
			// &#xA9;	-- copyright symbol, for example.
			while ( i<(int)str.length() )
			{
				outString->append( str.wc_str() + i, 1 );
				++i;
				if ( str[i] == ';' )
					break;
			}
		}
		else if ( c == '&' )
		{
			outString->append( entity[0].str, entity[0].strLength );
			++i;
		}
		else if ( c == '<' )
		{
			outString->append( entity[1].str, entity[1].strLength );
			++i;
		}
		else if ( c == '>' )
		{
			outString->append( entity[2].str, entity[2].strLength );
			++i;
		}
		else if ( c == '\"' )
		{
			outString->append( entity[3].str, entity[3].strLength );
			++i;
		}
		else if ( c == '\'' )
		{
			outString->append( entity[4].str, entity[4].strLength );
			++i;
		}
		else if ( c < 32 /*|| c > 126*/ )
		{
			// Easy pass at non-alpha/numeric/symbol
			// 127 is the delete key. Below 32 is symbolic.
			wchar_t buf[ 32 ];
			swprintf( buf, L"&#x%02X;", (unsigned) ( c & 0xff ) );
			outString->append( buf, wcslen(buf) );
			++i;
		}
		else
		{
			wchar_t realc = (wchar_t) c;
			outString->append( &realc, 1 );
			++i;
		}
	}
}


// <-- Strange class for a bug fix. Search for STL_STRING_BUG
#ifdef TIXML_USE_STL
TiXmlBase::StringToBuffer::StringToBuffer( const TIXML_STRING& str )
{
	buffer = new char[ str.length()+1 ];
	if ( buffer )
	{
		strcpy( buffer, str.c_str() );
	}
}
#endif


#ifdef TIXML_USE_STL
TiXmlBase::StringToBuffer::~StringToBuffer()
{
	delete [] buffer;
}
#endif
// End strange bug fix. -->


TiXmlNode::TiXmlNode( NodeType _type )
{
	parent = 0;
	type = _type;
	firstChild = 0;
	lastChild = 0;
	prev = 0;
	next = 0;
	userData = 0;
}


TiXmlNode::~TiXmlNode()
{
	TiXmlNode* node = firstChild;
	TiXmlNode* temp = 0;

	while ( node )
	{
		temp = node;
		node = node->next;
		delete temp;
	}	
}


void TiXmlNode::Clear()
{
	TiXmlNode* node = firstChild;
	TiXmlNode* temp = 0;

	while ( node )
	{
		temp = node;
		node = node->next;
		delete temp;
	}	

	firstChild = 0;
	lastChild = 0;
}


TiXmlNode* TiXmlNode::LinkEndChild( TiXmlNode* node )
{
	node->parent = this;

	node->prev = lastChild;
	node->next = 0;

	if ( lastChild )
		lastChild->next = node;
	else
		firstChild = node;			// it was an empty list.

	lastChild = node;
	return node;
}


TiXmlNode* TiXmlNode::InsertEndChild( const TiXmlNode& addThis )
{
	TiXmlNode* node = addThis.Clone();
	if ( !node )
		return 0;

	return LinkEndChild( node );
}


TiXmlNode* TiXmlNode::InsertBeforeChild( TiXmlNode* beforeThis, const TiXmlNode& addThis )
{	
	if ( !beforeThis || beforeThis->parent != this )
		return 0;

	TiXmlNode* node = addThis.Clone();
	if ( !node )
		return 0;
	node->parent = this;

	node->next = beforeThis;
	node->prev = beforeThis->prev;
	if ( beforeThis->prev )
	{
		beforeThis->prev->next = node;
	}
	else
	{
		assert( firstChild == beforeThis );
		firstChild = node;
	}
	beforeThis->prev = node;
	return node;
}


TiXmlNode* TiXmlNode::InsertAfterChild( TiXmlNode* afterThis, const TiXmlNode& addThis )
{
	if ( !afterThis || afterThis->parent != this )
		return 0;

	TiXmlNode* node = addThis.Clone();
	if ( !node )
		return 0;
	node->parent = this;

	node->prev = afterThis;
	node->next = afterThis->next;
	if ( afterThis->next )
	{
		afterThis->next->prev = node;
	}
	else
	{
		assert( lastChild == afterThis );
		lastChild = node;
	}
	afterThis->next = node;
	return node;
}


TiXmlNode* TiXmlNode::ReplaceChild( TiXmlNode* replaceThis, const TiXmlNode& withThis )
{
	if ( replaceThis->parent != this )
		return 0;

	TiXmlNode* node = withThis.Clone();
	if ( !node )
		return 0;

	node->next = replaceThis->next;
	node->prev = replaceThis->prev;

	if ( replaceThis->next )
		replaceThis->next->prev = node;
	else
		lastChild = node;

	if ( replaceThis->prev )
		replaceThis->prev->next = node;
	else
		firstChild = node;

	delete replaceThis;
	node->parent = this;
	return node;
}


bool TiXmlNode::RemoveChild( TiXmlNode* removeThis )
{
	if ( removeThis->parent != this )
	{	
		assert( 0 );
		return false;
	}

	if ( removeThis->next )
		removeThis->next->prev = removeThis->prev;
	else
		lastChild = removeThis->prev;

	if ( removeThis->prev )
		removeThis->prev->next = removeThis->next;
	else
		firstChild = removeThis->next;

	delete removeThis;
	return true;
}

TiXmlNode* TiXmlNode::FirstChild( const char * value ) const
{
	TiXmlNode* node;
	for ( node = firstChild; node; node = node->next )
	{
		if ( node->SValue() == TIXML_STRING( value ))
			return node;
	}
	return 0;
}

TiXmlNode* TiXmlNode::FirstChild( const wchar_t * value ) const
{
	TiXmlNode* node;
	for ( node = firstChild; node; node = node->next )
	{
		if ( node->SValue() == TIXML_STRING( value ))
			return node;
	}
	return 0;
}

TiXmlNode* TiXmlNode::LastChild( const char * value ) const
{
	TiXmlNode* node;
	for ( node = lastChild; node; node = node->prev )
	{
		if ( node->SValue() == TIXML_STRING (value))
			return node;
	}
	return 0;
}

TiXmlNode* TiXmlNode::LastChild( const wchar_t * value ) const
{
	TiXmlNode* node;
	for ( node = lastChild; node; node = node->prev )
	{
		if ( node->SValue() == TIXML_STRING (value))
			return node;
	}
	return 0;
}

TiXmlNode* TiXmlNode::IterateChildren( TiXmlNode* previous ) const
{
	if ( !previous )
	{
		return FirstChild();
	}
	else
	{
		assert( previous->parent == this );
		return previous->NextSibling();
	}
}

TiXmlNode* TiXmlNode::IterateChildren( const wchar_t * val, TiXmlNode* previous ) const
{
	if ( !previous )
	{
		return FirstChild( val );
	}
	else
	{
		assert( previous->parent == this );
		return previous->NextSibling( val );
	}
}

TiXmlNode* TiXmlNode::NextSibling( const char * value ) const
{
	TiXmlNode* node;
	for ( node = next; node; node = node->next )
	{
		if ( node->SValue() == TIXML_STRING (value))
			return node;
	}
	return 0;
}

TiXmlNode* TiXmlNode::NextSibling( const wchar_t * value ) const
{
	TiXmlNode* node;
	for ( node = next; node; node = node->next )
	{
		if ( node->SValue() == TIXML_STRING (value))
			return node;
	}
	return 0;
}


TiXmlNode* TiXmlNode::PreviousSibling( const char * value ) const
{
	TiXmlNode* node;
	for ( node = prev; node; node = node->prev )
	{
		if ( node->SValue() == TIXML_STRING (value))
			return node;
	}
	return 0;
}

TiXmlNode* TiXmlNode::PreviousSibling( const wchar_t * value ) const
{
	TiXmlNode* node;
	for ( node = prev; node; node = node->prev )
	{
		if ( node->SValue() == TIXML_STRING (value))
			return node;
	}
	return 0;
}

void TiXmlElement::RemoveAttribute( const char * name )
{
	TiXmlAttribute* node = attributeSet.Find( name );
	if ( node )
	{
		attributeSet.Remove( node );
		delete node;
	}
}

TiXmlElement* TiXmlNode::FirstChildElement() const
{
	TiXmlNode* node;

	for (	node = FirstChild();
	node;
	node = node->NextSibling() )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}

TiXmlElement* TiXmlNode::FirstChildElement( const char * value ) const
{
	TiXmlNode* node;

	for (	node = FirstChild( value );
	node;
	node = node->NextSibling( value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}

TiXmlElement* TiXmlNode::FirstChildElement( const wchar_t * value ) const
{
	TiXmlNode* node;

	for (	node = FirstChild( value );
	node;
	node = node->NextSibling( value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


TiXmlElement* TiXmlNode::NextSiblingElement() const
{
	TiXmlNode* node;

	for (	node = NextSibling();
	node;
	node = node->NextSibling() )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}

TiXmlElement* TiXmlNode::NextSiblingElement( const char * value ) const
{
	TiXmlNode* node;

	for (	node = NextSibling( value );
	node;
	node = node->NextSibling( value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}

TiXmlElement* TiXmlNode::NextSiblingElement( const wchar_t * value ) const
{
	TiXmlNode* node;

	for (	node = NextSibling( value );
	node;
	node = node->NextSibling( value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}



TiXmlDocument* TiXmlNode::GetDocument() const
{
	const TiXmlNode* node;

	for( node = this; node; node = node->parent )
	{
		if ( node->ToDocument() )
			return node->ToDocument();
	}
	return 0;
}


TiXmlElement::TiXmlElement (const char * _value)
: TiXmlNode( TiXmlNode::ELEMENT )
{
	firstChild = lastChild = 0;
	value = _value;
}

TiXmlElement::TiXmlElement (const wchar_t * _value)
: TiXmlNode( TiXmlNode::ELEMENT )
{
	firstChild = lastChild = 0;
	value = _value;
}

TiXmlElement::~TiXmlElement()
{
	while( attributeSet.First() )
	{
		TiXmlAttribute* node = attributeSet.First();
		attributeSet.Remove( node );
		delete node;
	}
}

const wchar_t * TiXmlElement::Attribute( const char * name ) const
{
	TiXmlAttribute* node = attributeSet.Find( name );

	if ( node )
		return node->Value();

	return 0;
}


const wchar_t * TiXmlElement::Attribute( const char * name, int* i ) const
{
	const wchar_t * s = Attribute( name );
	if ( i )
	{
		if ( s )
			*i = _wtoi( s );
		else
			*i = 0;
	}
	return s;
}


void TiXmlElement::SetAttribute( const char * name, int val )
{	
	wchar_t buf[64];
	swprintf( buf, L"%d", val );
	SetAttribute( name, buf );
}


void TiXmlElement::SetAttribute( const char * name, const char * value )
{
	TiXmlAttribute* node = attributeSet.Find( name );
	if ( node )
	{
		node->SetValue( value );
		return;
	}

	TiXmlAttribute* attrib = new TiXmlAttribute( name, value );
	if ( attrib )
	{
		attributeSet.Add( attrib );
	}
	else
	{
		TiXmlDocument* document = GetDocument();
		if ( document ) document->SetError( TIXML_ERROR_OUT_OF_MEMORY );
	}
}

void TiXmlElement::SetAttribute( const char * name, const wchar_t * value )
{
	TiXmlAttribute* node = attributeSet.Find( name );
	if ( node )
	{
		node->SetValue( value );
		return;
	}

	TiXmlAttribute* attrib = new TiXmlAttribute( name, value );
	if ( attrib )
	{
		attributeSet.Add( attrib );
	}
	else
	{
		TiXmlDocument* document = GetDocument();
		if ( document ) document->SetError( TIXML_ERROR_OUT_OF_MEMORY );
	}
}

void TiXmlElement::SetAttribute( const wchar_t * name, const wchar_t * value )
{
	TiXmlAttribute* node = attributeSet.Find( name );
	if ( node )
	{
		node->SetValue( value );
		return;
	}

	TiXmlAttribute* attrib = new TiXmlAttribute( name, value );
	if ( attrib )
	{
		attributeSet.Add( attrib );
	}
	else
	{
		TiXmlDocument* document = GetDocument();
		if ( document ) document->SetError( TIXML_ERROR_OUT_OF_MEMORY );
	}
}

void TiXmlElement::Print( FILE* cfile, int depth ) const
{
	int i;
	for ( i=0; i<depth; i++ )
	{
		fwprintf( cfile, L"    " );
	}

	fwprintf( cfile, L"<%s", value.wc_str() );

	TiXmlAttribute* attrib;
	for ( attrib = attributeSet.First(); attrib; attrib = attrib->Next() )
	{
		fwprintf( cfile, L" " );
		attrib->Print( cfile, depth );
	}

	// There are 3 different formatting approaches:
	// 1) An element without children is printed as a <foo /> node
	// 2) An element with only a text child is printed as <foo> text </foo>
	// 3) An element with children is printed on multiple lines.
	TiXmlNode* node;
	if ( !firstChild )
	{
		fwprintf( cfile, L" />" );
	}
	else if ( firstChild == lastChild && firstChild->ToText() )
	{
		fwprintf( cfile, L">" );
		firstChild->Print( cfile, depth + 1 );
		fwprintf( cfile, L"</%s>", value.wc_str() );
	}
	else
	{
		fwprintf( cfile, L">" );

		for ( node = firstChild; node; node=node->NextSibling() )
		{
			if ( !node->ToText() )
			{
				fwprintf( cfile, L"\n" );
			}
			node->Print( cfile, depth+1 );
		}
		fwprintf( cfile, L"\n" );
		for( i=0; i<depth; ++i )
		fwprintf( cfile, L"    " );
		fwprintf( cfile, L"</%s>", value.wc_str() );
	}
}

void TiXmlElement::StreamOut( TIXML_OSTREAM * stream ) const
{
	(*stream) << "<" << value;

	TiXmlAttribute* attrib;
	for ( attrib = attributeSet.First(); attrib; attrib = attrib->Next() )
	{	
		(*stream) << " ";
		attrib->StreamOut( stream );
	}

	// If this node has children, give it a closing tag. Else
	// make it an empty tag.
	TiXmlNode* node;
	if ( firstChild )
	{ 		
		(*stream) << ">";

		for ( node = firstChild; node; node=node->NextSibling() )
		{
			node->StreamOut( stream );
		}
		(*stream) << "</" << value << ">";
	}
	else
	{
		(*stream) << " />";
	}
}

TiXmlNode* TiXmlElement::Clone() const
{
	TiXmlElement* clone = new TiXmlElement( Value() );
	if ( !clone )
		return 0;

	CopyToClone( clone );

	// Clone the attributes, then clone the children.
	TiXmlAttribute* attribute = 0;
	for(	attribute = attributeSet.First();
	attribute;
	attribute = attribute->Next() )
	{
		clone->SetAttribute( attribute->Name(), attribute->Value() );
	}

	TiXmlNode* node = 0;
	for ( node = firstChild; node; node = node->NextSibling() )
	{
		clone->LinkEndChild( node->Clone() );
	}
	return clone;
}


TiXmlDocument::TiXmlDocument() : TiXmlNode( TiXmlNode::DOCUMENT )
{
	error = false;
	//	ignoreWhiteSpace = true;
}

TiXmlDocument::TiXmlDocument( const char * documentName ) : TiXmlNode( TiXmlNode::DOCUMENT )
{
	//	ignoreWhiteSpace = true;
	value = documentName;
	error = false;
}

bool TiXmlDocument::LoadFile()
{
	#ifdef TIXML_USE_STL
		// See STL_STRING_BUG below.
		StringToBuffer buf( value );

		if ( buf.buffer && LoadFile( buf.buffer ) )
			return true;
	#else
		if (LoadFile(value.wc_str()))
			return true;
	#endif

	return false;
}


bool TiXmlDocument::SaveFile() const
{
	#ifdef TIXML_USE_STL
		// See STL_STRING_BUG below.
		StringToBuffer buf( value );

		if ( buf.buffer && SaveFile( buf.buffer ) )
			return true;
	#else
		if (SaveFile(value.wc_str()))
			return true;
	#endif

	return false;
}

bool TiXmlDocument::LoadFile( const char* filename )
{
	value = filename;
	return LoadFile();
}

void TiXmlDocument::SwapBytes(unsigned char* buffer, long length)
{
	for (long i = 0; i < length; i += 2)
	{
		unsigned char c = buffer[i];
		buffer[i] = buffer[i + 1];
		buffer[i + 1] = c;
	}
}

void TiXmlDocument::ConvertFromWideChar(unsigned char* buffer, long length, TiXmlString& result)
{
	result.reserve(length / 2 + 1);
	for (long i = 0; i < length / 2; i++)
	{
		wchar_t c = ((unsigned short*)buffer)[i];
		result += c;
	}
}

// read in the file as a widechar string
// accept both multibyte & widechar files
// NOTE: 32bit widechar files are NOT correctly handled
bool TiXmlDocument::LoadFile(FILE* file, TiXmlString& result)
{
	long length;
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (length <= 0) return false;

	unsigned char* buffer = new unsigned char[length + 1];
	assert(buffer);

	if (fread(buffer, 1, length, file) != length)
	{
		SetError( TIXML_ERROR_OPENING_FILE );
		delete buffer;
		return false;
	}

	if (length & 1)
	{
		// must be multibyte
		buffer[length] = 0;
		result = (char*) buffer;
	}
	else if
		(
			(!buffer[0] || buffer[0] >= 0xfe) &&
			(!buffer[1] || buffer[1] >= 0xfe)
		)
	{
		// appears to be widechar
		const unsigned short swappedBOM = 0xfffe;
		const unsigned short hiBits = 0xff80;

		if (((unsigned short*)buffer)[0] == swappedBOM)
		{
			SwapBytes(buffer, length);
		}
		else if
			(
			(!buffer[0] || !buffer[1]) &&
			(((unsigned short*)buffer)[0] & hiBits) // the first char should be from the ASCII-7 charset
			)
		{
			SwapBytes(buffer, length);
		}
		if (buffer[0] >= 0xfe && buffer[1] >= 0xfe)
		{
			buffer += 2;
			length -= 2;
		}
		ConvertFromWideChar(buffer, length, result);
	}
	else
	{
		// appears to be multibyte
		buffer[length] = 0;
		result = (char*) buffer;
	}

	return true;
}

bool TiXmlDocument::LoadFile( const wchar_t* filename )
{
	// Delete the existing data:
	Clear();

	// There was a really terrifying little bug here. The code:
	//		value = filename
	// in the STL case, cause the assignment method of the std::string to
	// be called. What is strange, is that the std::string had the same
	// address as it's c_str() method, and so bad things happen. Looks
	// like a bug in the Microsoft STL implementation.
	// See STL_STRING_BUG above.
	// Fixed with the StringToBuffer class.
	value = filename;

	FILE* file = _wfopen( value.wc_str (), L"rb" );

	if ( file )
	{
		/*
		// Get the file size, so we can pre-allocate the string. HUGE speed impact.
		long length = 0;
		fseek( file, 0, SEEK_END );
		length = ftell( file );
		fseek( file, 0, SEEK_SET );

		// Strange case, but good to handle up front.
		if ( length == 0 )
		{
			fclose( file );
			return false;
		}

		// If we have a file, assume it is all one big XML file, and read it in.
		// The document parser may decide the document ends sooner than the entire file, however.
		TIXML_STRING data;
		data.reserve( length );

		const int BUF_SIZE = 2048;
		wchar_t buf[BUF_SIZE];

		while( fgetws( buf, BUF_SIZE, file ) )
		{
			data += buf;
		}
		*/

		TiXmlString data;
		if (!LoadFile(file, data))
		{
			return false;
		}

		fclose( file );

		Parse( data.wc_str() );
		if (  !Error() )
		{
			return true;
		}
	}
	SetError( TIXML_ERROR_OPENING_FILE );
	return false;
}

bool TiXmlDocument::SaveFile( const char * filename ) const
{
	// The old c stuff lives on...
	FILE* fp = fopen( filename, "wb" );
	if ( fp )
	{
		static unsigned short bom = 0xfeff;

		fprintf(fp, "%c%c", ((char*) &bom)[0], ((char*) &bom)[1]);
		Print( fp, 0 );
		fclose( fp );
		return true;
	}
	return false;
}

bool TiXmlDocument::SaveFile( const wchar_t * filename ) const
{
	// The old c stuff lives on...
	FILE* fp = _wfopen( filename, L"wb" );
	if ( fp )
	{
		static unsigned short bom = 0xfeff;

		fprintf(fp, "%c%c", ((char*) &bom)[0], ((char*) &bom)[1]);
		Print( fp, 0 );
		fclose( fp );
		return true;
	}
	return false;
}


TiXmlNode* TiXmlDocument::Clone() const
{
	TiXmlDocument* clone = new TiXmlDocument();
	if ( !clone )
		return 0;

	CopyToClone( clone );
	clone->error = error;
	clone->errorDesc = errorDesc.wc_str ();

	TiXmlNode* node = 0;
	for ( node = firstChild; node; node = node->NextSibling() )
	{
		clone->LinkEndChild( node->Clone() );
	}
	return clone;
}


void TiXmlDocument::Print( FILE* cfile, int depth ) const
{
	TiXmlNode* node;
	for ( node=FirstChild(); node; node=node->NextSibling() )
	{
		node->Print( cfile, depth );
		fwprintf( cfile, L"\n" );
	}
}

void TiXmlDocument::StreamOut( TIXML_OSTREAM * out ) const
{
	TiXmlNode* node;
	for ( node=FirstChild(); node; node=node->NextSibling() )
	{
		node->StreamOut( out );

		// Special rule for streams: stop after the root element.
		// The stream in code will only read one element, so don't
		// write more than one.
		if ( node->ToElement() )
			break;
	}
}

TiXmlAttribute* TiXmlAttribute::Next() const
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( next->value.empty() && next->name.empty() )
		return 0;
	return next;
}


TiXmlAttribute* TiXmlAttribute::Previous() const
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( prev->value.empty() && prev->name.empty() )
		return 0;
	return prev;
}


void TiXmlAttribute::Print( FILE* cfile, int /*depth*/ ) const
{
	TIXML_STRING n, v;

	PutString( Name(), &n );
	PutString( Value(), &v );

	if (value.find ('\"') == TIXML_STRING::npos)
		fwprintf (cfile, L"%s=\"%s\"", n.wc_str(), v.wc_str() );
	else
		fwprintf (cfile, L"%s='%s'", n.wc_str(), v.wc_str() );
}


void TiXmlAttribute::StreamOut( TIXML_OSTREAM * stream ) const
{
	if (value.find( '\"' ) != TIXML_STRING::npos)
	{
		PutString( name, stream );
		(*stream) << "=" << "'";
		PutString( value, stream );
		(*stream) << "'";
	}
	else
	{
		PutString( name, stream );
		(*stream) << "=" << "\"";
		PutString( value, stream );
		(*stream) << "\"";
	}
}

void TiXmlAttribute::SetIntValue( int value )
{
	wchar_t buf [64];
	swprintf (buf, L"%d", value);
	SetValue (buf);
}

void TiXmlAttribute::SetDoubleValue( double value )
{
	wchar_t buf [64];
	swprintf (buf, L"%lf", value);
	SetValue (buf);
}

const int TiXmlAttribute::IntValue() const
{
	return _wtoi (value.wc_str ());
}

const double  TiXmlAttribute::DoubleValue() const
{
	return _wtof (value.wc_str ());
}

void TiXmlComment::Print( FILE* cfile, int depth ) const
{
	for ( int i=0; i<depth; i++ )
	{
		fputs( "    ", cfile );
	}
	fwprintf( cfile, L"<!--%s-->", value.wc_str() );
}

void TiXmlComment::StreamOut( TIXML_OSTREAM * stream ) const
{
	(*stream) << "<!--";
	PutString( value, stream );
	(*stream) << "-->";
}

TiXmlNode* TiXmlComment::Clone() const
{
	TiXmlComment* clone = new TiXmlComment();

	if ( !clone )
		return 0;

	CopyToClone( clone );
	return clone;
}


void TiXmlText::Print( FILE* cfile, int /*depth*/ ) const
{
	TIXML_STRING buffer;
	PutString( value, &buffer );
	fwprintf( cfile, L"%s", buffer.wc_str() );
}


void TiXmlText::StreamOut( TIXML_OSTREAM * stream ) const
{
	PutString( value, stream );
}


TiXmlNode* TiXmlText::Clone() const
{	
	TiXmlText* clone = 0;
	clone = new TiXmlText( L"" );

	if ( !clone )
		return 0;

	CopyToClone( clone );
	return clone;
}


TiXmlDeclaration::TiXmlDeclaration( const wchar_t * _version,
	const wchar_t * _encoding,
	const wchar_t * _standalone )
: TiXmlNode( TiXmlNode::DECLARATION )
{
	version = _version;
	encoding = _encoding;
	standalone = _standalone;
}

TiXmlDeclaration::TiXmlDeclaration( const char * _version,
	const char * _encoding,
	const char * _standalone )
: TiXmlNode( TiXmlNode::DECLARATION )
{
	version = _version;
	encoding = _encoding;
	standalone = _standalone;
}


void TiXmlDeclaration::Print( FILE* cfile, int /*depth*/ ) const
{
	fwprintf (cfile, L"<?xml ");

	if ( !version.empty() )
		fwprintf (cfile, L"version=\"%s\" ", version.wc_str ());
	/*
	if ( !encoding.empty() )
		fwprintf (cfile, L"encoding=\"%s\" ", encoding.wc_str ());
	*/
	if ( !standalone.empty() )
		fwprintf (cfile, L"standalone=\"%s\" ", standalone.wc_str ());
	fwprintf (cfile, L"?>");
}

void TiXmlDeclaration::StreamOut( TIXML_OSTREAM * stream ) const
{
	(*stream) << "<?xml ";

	if ( !version.empty() )
	{
		(*stream) << "version=\"";
		PutString( version, stream );
		(*stream) << "\" ";
	}
	if ( !encoding.empty() )
	{
		(*stream) << "encoding=\"";
		PutString( encoding, stream );
		(*stream ) << "\" ";
	}
	if ( !standalone.empty() )
	{
		(*stream) << "standalone=\"";
		PutString( standalone, stream );
		(*stream) << "\" ";
	}
	(*stream) << "?>";
}

TiXmlNode* TiXmlDeclaration::Clone() const
{	
	TiXmlDeclaration* clone = new TiXmlDeclaration();

	if ( !clone )
		return 0;

	CopyToClone( clone );
	clone->version = version;
	clone->encoding = encoding;
	clone->standalone = standalone;
	return clone;
}


void TiXmlUnknown::Print( FILE* cfile, int depth ) const
{
	for ( int i=0; i<depth; i++ )
		fwprintf( cfile, L"    " );
	fwprintf( cfile, L"%s", value.wc_str() );
}

void TiXmlUnknown::StreamOut( TIXML_OSTREAM * stream ) const
{
	(*stream) << "<" << value << ">";		// Don't use entities hear! It is unknown.
}

TiXmlNode* TiXmlUnknown::Clone() const
{
	TiXmlUnknown* clone = new TiXmlUnknown();

	if ( !clone )
		return 0;

	CopyToClone( clone );
	return clone;
}


TiXmlAttributeSet::TiXmlAttributeSet()
{
	sentinel.next = &sentinel;
	sentinel.prev = &sentinel;
}


TiXmlAttributeSet::~TiXmlAttributeSet()
{
	assert( sentinel.next == &sentinel );
	assert( sentinel.prev == &sentinel );
}


void TiXmlAttributeSet::Add( TiXmlAttribute* addMe )
{
	assert( !Find( addMe->Name() ) );	// Shouldn't be multiply adding to the set.

	addMe->next = &sentinel;
	addMe->prev = sentinel.prev;

	sentinel.prev->next = addMe;
	sentinel.prev      = addMe;
}

void TiXmlAttributeSet::Remove( TiXmlAttribute* removeMe )
{
	TiXmlAttribute* node;

	for( node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node == removeMe )
		{
			node->prev->next = node->next;
			node->next->prev = node->prev;
			node->next = 0;
			node->prev = 0;
			return;
		}
	}
	assert( 0 );		// we tried to remove a non-linked attribute.
}

TiXmlAttribute*	TiXmlAttributeSet::Find( const char * name ) const
{
	TiXmlAttribute* node;

	for( node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node->name == name )
			return node;
	}
	return 0;
}

TiXmlAttribute*	TiXmlAttributeSet::Find( const wchar_t * name ) const
{
	TiXmlAttribute* node;

	for( node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node->name == name )
			return node;
	}
	return 0;
}



#ifdef TIXML_USE_STL	
TIXML_ISTREAM & operator >> (TIXML_ISTREAM & in, TiXmlNode & base)
{
	TIXML_STRING tag;
	tag.reserve( 8 * 1000 );
	base.StreamIn( &in, &tag );

	base.Parse( tag.c_str() );
	return in;
}
#endif


TIXML_OSTREAM & operator<< (TIXML_OSTREAM & out, const TiXmlNode & base)
{
	base.StreamOut (& out);
	return out;
}