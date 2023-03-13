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
#ifndef MIPI_SYST_COLLATERL_H_included
#define MIPI_SYST_COLLATERL_H_included

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <algorithm>

#include "pugixml.hpp"
#include "mipi_syst_printer.h"
#include "mipi_syst_guid.h"

MIPI_SYST_NAMESPACE_BEGIN
class guid;

class collateral {
public:
	const std::string& getName() const { return m_name;}
	const std::string& getFileName() const { return m_file; }

	/// Check if the given guid matches one of the catalog guid/mask pairs
	///
	/// @param guid to check catalog for
	/// @param build  client build number
	/// @return true if catalog matches, false otherwise
	///
	bool match(const guid & g, const uint64_t build) const;

	static std::vector<collateral*> parseXml(const std::string& filename);

	struct sourcepos {
		sourcepos(uint32_t file, uint32_t line) :
			m_file(file), m_line(line) {};

		uint32_t m_file = 0;
		uint32_t m_line = 0;
	};

	/// Message entry for ID->MSG catalogs
	///
	template<typename T> struct catalogentry : public sourcepos {
		std::string msg;
		T mask;

		catalogentry(std::string s ="",  T mask = (T)-1, uint32_t file = 0, uint32_t line=0)
			: sourcepos(file, line), msg(s)
		{}

		bool operator!=(const catalogentry& other) {
			return msg != other.msg;
		}
	};

	/// Container for (key:mask:value tuples
	///
	template<typename K, typename V> struct masked_item
	{
		masked_item() {}
		masked_item(const K& k, const K& m, const V& v) :
			m_key(k), m_mask(m), m_value(v) {}

		K& key()   { return m_key; }
		K& mask()  {return m_mask;}
		V& value() {return m_value;}

		const K& key()   const { return m_key; }
		const K& mask()  const { return m_mask; }
		const V& value() const { return m_value; }

	private:
		K m_key;
		K m_mask;
		V m_value;
	};

	template<typename K, typename V>
	class masked_vector : public std::vector<masked_item<K, V>>
	{
	public:
		typedef std::vector<masked_item<K, V>> base_type;

		/// Find key in item vector using item mask when comparing (const)
		///
		/// @param k key to look for
		/// @return iterator to matching element or end() if not found
		///
		typename base_type::const_iterator
		find(const K& k) const
		{
			return std::find_if(base_type::begin(), base_type::end(),
				[& k](const masked_item<K, V>& v)
				{
					return (v.key() & v.mask()) == (k & v.mask());
				}
			);
		}


		/// Find key in item vector using item mask when comparing
		///
		/// @param k key to look for
		///  @return iterator to matching element or end() if not found
		///
		typename base_type::iterator
		find(const K& k)
		{
			return std::find_if(base_type::begin(), base_type::end(),
				[&k]( masked_item<K, V>& v)
			{
				return (v.key() & v.mask()) == (k & v.mask());
			}
			);
		}
	};

	/// container for ID <-> value mapping sections in collateral
	///
	using guids      =  masked_vector<guid, std::string>;
	using builds     = masked_vector<uint64_t, std::string>;
	using srcfiles   = masked_vector<uint32_t, std::string>;
	using origins    = masked_vector<uint32_t, std::string>;
	using writetypes = masked_vector<uint8_t, std::string>;
	using catalog32  = masked_vector<uint32_t, catalogentry<uint32_t>>;
	using catalog64  = masked_vector<uint64_t, catalogentry<uint64_t>>;

	template<typename T> const catalogentry<T> * getShortEntry(T id) const;
	template<typename T> const catalogentry<T> * getCatalogEntry(T id) const;

	const std::string  * getSourceFile(uint32_t id) const;
	const std::string  * getWriteType(uint8_t id) const;


	/// Parse collateral from XML
	///
	collateral(pugi::xml_node& node, const std::string& file);

	template<typename K, typename V>
	void parseMaskedItems(
		pugi::xml_node root,
		const char * tag,
		masked_vector<K, V>& dest);
private:
	std::string m_file;       /**< originating file name */

	std::string m_name;       /**< catalog name */
	guids       m_guids;      /**< Guids where the collateral is valid    */
	builds      m_builds;     /**< builds where the collateral is valid   */
	catalog32   m_msgs32;     /**< message catalog with 32bit ID's        */
	catalog64   m_msgs64;     /**< message catalog with 64bit ID's        */
	catalog32   m_shorts32;   /**< short event id to message mapping      */
	catalog64   m_shorts64;   /**< short event id to message mapping      */
	writetypes  m_writeTypes; /**< write definitions                      */
	srcfiles    m_files;      /**< file catalog                           */
	origins     m_modules;    /**< module id to name mappings             */


private:
	/* no copy, assign */
	collateral() = delete;
	collateral(const collateral&) = delete;
	collateral& operator=(const collateral&) = delete;
};

MIPI_SYST_NAMESPACE_END

#endif // MIPI_SYST_COLLATERL_H_included
