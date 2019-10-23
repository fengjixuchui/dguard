//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "xmlParser.h"
#include <boost/foreach.hpp>

namespace parser
{
	namespace xml
	{
		std::map<std::string, FullNode> XmlParser::get1stKeyValues(const std::string& _filepath)
		{
			std::map<std::string, FullNode> result;

			try
			{
				boost::property_tree::ptree propertyTree;
				readXml(_filepath, propertyTree);
				

				for (auto &v : propertyTree)
				{
					FullNode node;
					if (readNode(v, node))
					{
						result[node.m_name] = node;
					}
				}
			}
			catch (boost::property_tree::xml_parser_error)
			{
				return std::map<std::string, FullNode>();
			}

			return result;
		}

		bool XmlParser::readXml(const std::string& _filepath, boost::property_tree::ptree& _propertyTree)
		{
			std::stringstream xmlData;
			if (readFile(_filepath, xmlData))
			{
				boost::property_tree::read_xml(xmlData,
											   _propertyTree,
											   // Зачитка без комментариев
											   boost::property_tree::xml_parser::no_comments);
				return true;
			}
			return false;
		}

		bool XmlParser::get1stValues(const std::string& _filepath, const std::string& _key, std::set<std::string>& _outValues)
		{
			try
			{
				boost::property_tree::ptree propertyTree;
				readXml(_filepath, propertyTree);
				//propertyTree.get_value<std::string>(_key);
				//auto it = propertyTree.find(_key);
				//if (it != propertyTree.end())

				// Некрасиво, покурить boost и зарефакторить
				for (auto &v: propertyTree)
				{
					if (_key == v.first)
					{
						_outValues.insert(v.second.get_value<std::string>());
					}
				}
			}
			catch (boost::property_tree::xml_parser_error)
			{
				return false;
			}

			return true;
		}

		bool XmlParser::set1stValue(const std::string& _filepath, const std::string& _key, const std::string& _value)
		{
			try
			{
				boost::property_tree::ptree propertyTree;
				readXml(_filepath, propertyTree);

				propertyTree.put(_key, _value);

				writeXml(_filepath, propertyTree);
			}
			catch (...)
			{
				return false;
			}

			return true;
		}

		bool XmlParser::writeXml(const std::string& _filepath, const boost::property_tree::ptree& _propertyTree)
		{
			// 			std::stringstream xmlData;
			// 			boost::property_tree::write_xml(xmlData,
			// 											_propertyTree,
			// 											// Запись xml'ки красиво, удобочитаемо, - в столбик
			// 											boost::property_tree::xml_writer_make_settings<boost::property_tree::ptree::key_type::value_type>('\n', 1));
			// 
			// 			return writeFile(_filepath, xmlData);


			//
			// Took it here - https://stackoverflow.com/questions/29370713/compilation-error-with-boostproperty-treexml-writer-settings
			//
			std::ofstream file(_filepath, std::ofstream::out | std::ofstream::trunc);

			if (!file.is_open())
			{
				return false;
			}

			boost::property_tree::write_xml(
				file,
				_propertyTree,
				boost::property_tree::xml_writer_make_settings<std::string>('\n', 1));

			file.close();

			return true;
		}

		bool XmlParser::readNode(const std::pair<std::string, boost::property_tree::ptree>& _subTree, FullNode& _node)
		{
			_node.m_name = _subTree.first;
			_node.m_value = _subTree.second.get_value<std::string>();
			try
			{
				auto childs = _subTree.second.get_child("<xmlattr>");
				for (const auto& item : childs)
				{
					auto name = item.first;
					auto value = item.second.get_value<std::string>();
					_node.m_attributes.insert(std::make_pair(name, value));
				}
			}
			catch (...)
			{
				// нету аттрибутов
			}

			return true;
		}

		bool XmlParser::readFile(const std::string& _filepath, std::stringstream& _xmlData)
		{
			std::ifstream file(_filepath);
			if (file.is_open())
			{
				std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(_xmlData));
				return true;
			}
			return false;
		}

		bool XmlParser::writeFile(const std::string& _filepath, const std::stringstream& _data)
		{
			std::ofstream ofs(_filepath, std::ofstream::out | std::ofstream::trunc);
			if (ofs.is_open())
			{
				ofs.write(_data.str().c_str(), _data.str().size());
				return true;
			}

			return false;
		}

		bool XmlParser::rewrite(const std::string& _filepath, const std::map<std::string, FullNode>& _newData)
		{
			try
			{
				boost::property_tree::ptree propertyTree;
				for (const auto& item : _newData)
				{
					boost::property_tree::ptree node;
					propertyTree.add_child(item.first, node);
				}

				writeXml(_filepath, propertyTree);

				return true;
			}
			catch (...)
			{
				return false;
			}
		}

		bool XmlParser::insertOrUpdate(const std::string& _filepath, const std::map<std::string, FullNode>& _dataSet)
		{
			try
			{
				boost::property_tree::ptree propertyTree;
				readXml(_filepath, propertyTree);

				for (const auto& item : _dataSet)
				{
					const auto& attributes = item.second.m_attributes;
					auto& node = propertyTree.put(item.second.m_name, item.second.m_value);
					
					for (const auto& attr : attributes)
					{
						node.put(std::string("<xmlattr>.") + attr.first, attr.second);
						node.put(std::string("<xmlattr>.") + attr.first, attr.second);
					}
				}

				writeXml(_filepath, propertyTree);

				return true;
			}
			catch (...)
			{
				return false;
			}
		}
	}
}
