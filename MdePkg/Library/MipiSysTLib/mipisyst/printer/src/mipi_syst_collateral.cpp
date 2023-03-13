/*
 Copyright (c) 2018, MIPI Alliance, Inc. 
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the
   distribution.

 * Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * Contributors:
 * Norbert Schulz (Intel Corporation) - Initial API and implementation
 */

#include <iomanip>
#include <sstream>
#include <iostream>
#include <type_traits>

#include "mipi_syst_collateral.h"
#include "mipi_syst_guid.h"

MIPI_SYST_NAMESPACE_BEGIN

const char tagCollateral[]     = "syst:Collateral";
const char tagClient[]         = "syst:Client";
const char tagGuids[]          = "syst:Guids";
const char tagGuid[]           = "syst:Guid";
const char tagBuilds[]         = "syst:Builds";
const char tagBuild[]          = "syst:Build";
const char tagCatalog32[]      = "syst:Catalog32";
const char tagCatalog64[]      = "syst:Catalog64";
const char tagSourceFiles[]    = "syst:SourceFiles";
const char tagFile[]           = "syst:File";
const char tagShort32[]        = "syst:Short32";
const char tagShort64[]        = "syst:Short64";
const char tagWrite[]          = "syst:Write";
const char tagProtocol[]       = "syst:Protocol";
const char tagModules[]        = "syst:Modules";
const char tagModule[]         = "syst:Module";
const char tagID[]             = "ID";
const char tagMask[]           = "Mask";
const char tagFileAttr[]       = "File";
const char tagLine[]           = "Line";
const char tagFormat[]         = "syst:Format";


std::vector<collateral*> collateral::parseXml(const std::string& filename)
{
	pugi::xml_document doc;
	pugi::xml_parse_result scan_result(doc.load_file(filename.c_str()));
	std::stringstream sstr;

	if (!scan_result) {
		sstr
			<< "XML parser error on file "
			<< filename
			<< " : "
			<< scan_result.description();

		throw std::invalid_argument(sstr.str());
	}

	std::vector<collateral*> result;

	for(auto client(doc.child(tagCollateral).child(tagClient));
		client;
		client = client.next_sibling(tagClient))
	{
		result.push_back(new collateral(client, filename));
	}
	return result;
}


collateral::collateral(pugi::xml_node& node, const std::string& file) :
	m_file(file),
	m_name(node.attribute("Name").as_string())
{
	// Scan id <-> item mappings
	//
	parseMaskedItems(node.child(tagGuids), tagGuid, m_guids);
	parseMaskedItems(node.child(tagBuilds), tagBuild, m_builds);
	parseMaskedItems(node.child(tagSourceFiles), tagFile, m_files);
	parseMaskedItems(node.child(tagModules), tagModule, m_modules);
	parseMaskedItems(node.child(tagCatalog32), tagFormat, m_msgs32);
	parseMaskedItems(node.child(tagCatalog64), tagFormat, m_msgs64);
	parseMaskedItems(node.child(tagShort32), tagFormat, m_shorts32);
	parseMaskedItems(node.child(tagShort64), tagFormat, m_shorts64);
	parseMaskedItems(node.child(tagWrite), tagProtocol, m_writeTypes);
}

bool collateral::match(const guid& g, uint64_t build) const
{
	for (auto& it : m_guids) {
		if ((g & it.mask()) == (it.key() & it.mask()))
		{
			if (build != 0x0) {
				// find collateral with matching build number
				//
				if (m_builds.find(build) != m_builds.end())
				{
					return true;
				}
			} else {
				return true;
			}
		}
	}

	return false;
}

template<> const collateral::catalogentry<uint32_t> * collateral::getCatalogEntry<>(uint32_t id) const
{
	auto hit = m_msgs32.find(id);
	return (hit != m_msgs32.end()) ? &hit->value() : nullptr;
}

template<> const collateral::catalogentry<uint64_t> * collateral::getCatalogEntry<>(uint64_t id) const
{
	auto hit = m_msgs64.find(id);
	return (hit != m_msgs64.end()) ? &hit->value() : nullptr;
}
const std::string * collateral::getSourceFile(uint32_t id) const
{
	auto hit = m_files.find(id);
	return (hit != m_files.end()) ? &hit->value() : nullptr;
}

const std::string * collateral::getWriteType(uint8_t id) const
{
	auto hit(m_writeTypes.find(id));
	return (hit != m_writeTypes.end()) ? &hit->value() : nullptr;
}

template<> const collateral::catalogentry<uint32_t> * collateral::getShortEntry<>(uint32_t id) const
{
	auto hit(m_shorts32.find(id));
	return (hit != m_shorts32.end()) ? &hit->value() : nullptr;
}
template<> const collateral::catalogentry<uint64_t> * collateral::getShortEntry<>(uint64_t id) const
{
	auto hit(m_shorts64.find(id));
	return (hit != m_shorts64.end()) ? &hit->value() : nullptr;
}

template<typename K> bool parse(K& dest, const std::string& s)
{
	return stringToNum<K>(s, dest);
}

template<> bool parse<guid>(guid& g, const std::string& s)
{
	return g.parse(s);
}

template<typename K> K nomask() { return (K)-1; }
template<> guid nomask<guid>() { return guid("{ffffffff-ffff-ffff-ffff-ffffffffffff}"); }

template<typename T> std::ostream & operator<<(std::ostream &os, const collateral::catalogentry<T>& p)
{
	return os << p.msg;
}

template<typename K, typename V>
void collateral::parseMaskedItems(
	pugi::xml_node root, const char * tag, masked_vector<K, V>& dest)
{
	for (pugi::xml_node item(root.child(tag));
		item;
		item = item.next_sibling(tag))
	{
		masked_item<K, V> data;

		std::string keyStr(item.attribute(tagID).as_string());
		std::string val(item.child_value());

		if (!parse<K>(data.key(), keyStr)) {
			std::stringstream what;
			what <<
				"Malformed " << tag << "entry ID=\"" << keyStr <<
				"\" value =\"" << val << "\"" <<
				" for catalog " << m_name;
			throw std::invalid_argument(what.str());
		}

		std::string maskStr(item.attribute(tagMask).as_string());
		if (!maskStr.empty()) {
			if (!parse<K>(data.mask(), maskStr)) {
				std::stringstream what;
				what <<
					"Malformed " << tag << "entry Mask=\"" << maskStr <<
					"\" value =\"" << val << "\"" <<
					" for catalog " << m_name;
				throw std::invalid_argument(what.str());
			}
		} else {
			data.mask() = nomask<K>();
		}

		data.value() = val;

		// scan option file/line data if value supports source positions
		//
		if (std::is_base_of<catalogentry<K>, V>::value) {
			catalogentry<K> * entry((catalogentry<K>*)&data.value());
			entry->mask = data.mask();

			std::string file(item.attribute(tagFileAttr).as_string());
			std::string line(item.attribute(tagLine).as_string());

			if (!file.empty()) {
				if (!parse(entry->m_file, file)) {
					std::stringstream what;
					what <<
						"Malformed File attribute " << file <<
						"in section " << tag <<
						" for catalog " << m_name;
					throw std::invalid_argument(what.str());
				}
			}

			if (!line.empty()) {
				if (!parse(entry->m_line, line)) {
					std::stringstream what;
					what <<
						"Malformed Line attribute " << line <<
						"in section " << tag <<
						" for catalog " << m_name;
					throw std::invalid_argument(what.str());
				}
			}
		}

		auto it = dest.find(data.key());
		if (it != dest.end() && it->value() != val) {
			std::cerr <<
				"Overwriting  ID" << data.key() << " -> " << val <<
				" old value: " << it->value() << std::endl;
			*it = data;
		}
		else {
			dest.push_back(data);
		}
	}
}

/** Helper to parse a GUID "{...}" string into the binary representation.
* @param str GUID ascii string "{xxxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
* @param guid resulting guid
* @return true if guid could be parserd
*/
bool guid::parse(const std::string& str)
{
	if (
		(str.length() != sizeof("{00000000-0000-0000-0000-000000000000}")-1) ||
		str[0]!= '{' ||str[37] != '}' ||
		str[9] != '-' || str[14] != '-' || str[19] != '-' || str[24] != '-'
	   )
	{
		return false;
	}

	// parse the byte values;
	//
	const char * p(str.c_str()+1);

	for (int i(0); i < 16; ++i, p += 2) {
		if (i == 4 || i == 6|| i == 8 || i == 10) {
			p++;  // skip "-"
		}
		std::stringstream sstr(std::string(p, 2));
		uint16_t temp;

		if (! (sstr >> std::hex >>temp)) {
			return false;
		}
		u.b[i] = (uint8_t)(temp);
	}

	return true;
}

std::ostream& operator<<(std::ostream& os, const guid& g)
{
	auto flags(os.flags());


	os << '{';

	for (int i(0); i < 16; ++i) {
		os <<  std::setfill('0') << std::noshowbase << std::hex << std::setw(2)
		   << (uint16_t)g.u.b[i];

		if (i == 3 || i == 5|| i == 7 || i == 9) {
			os << '-';
		}
	}

	os << '}';

	os.flags(flags);
	return os;
}

MIPI_SYST_NAMESPACE_END