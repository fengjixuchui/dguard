
//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <sstream>
#include <boost/property_tree/xml_parser.hpp>
#include <set>
#include <map>


namespace parser
{
	namespace xml
	{
		struct FullNode
		{
			std::string m_name;
			std::string m_value;
			std::map<std::string /*name of attribute*/, std::string /*value of attribute*/> m_attributes;
		};

		class XmlParser
		{
		public:
			// Gets all nodes named as 'key' of std::map from specified xml-file
			static std::map<std::string, FullNode> get1stKeyValues(const std::string& _filepath);

			// Reads all entries from 1st level of deep.
			static bool get1stValues(const std::string& _filepath, const std::string& _key, std::set<std::string>& _outValues);

			// Set value for present key or add value for new key without attributes.
			static bool set1stValue(const std::string& _filepath, const std::string& _key, const std::string& _value);

			// Insert or updates with key-value set
			static bool insertOrUpdate(const std::string& _filepath, const std::map<std::string, FullNode>& _dataSet);

			// Re-writes data with new key-values.
			static bool rewrite(const std::string& _filepath, const std::map<std::string, FullNode>& _newData);

		private:
			// Gets data from file.
			static bool readFile(const std::string& _filepath, std::stringstream& _xmlData);

			// Puts data to file.
			static bool writeFile(const std::string& _filepath, const std::stringstream& _data);

			// read all tree from file
			static bool readXml(const std::string& _filepath, boost::property_tree::ptree& _propertyTree);

			// write all tree to file
			static bool writeXml(const std::string& _filepath, const boost::property_tree::ptree& _propertyTree);

			// read property subtree to FullNode struct
			static bool readNode(const std::pair<std::string, boost::property_tree::ptree>& _subTree, FullNode& _node);
		};
	}
}
