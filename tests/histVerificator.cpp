#include "histVerificator.h"
#include "common.h"

#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>

static const char separatorHist{ '\t' }, separatorFields{ ',' }, separatorAttribute{ ':' };
static void parseExcelFormat(std::ifstream& outputFile, std::unordered_map<std::string, histVerificator::histInfo>& foundInfos);
static void parseFollowFormat(std::ifstream& outputFile, std::unordered_map<std::string, histVerificator::histInfo>& foundInfos);

bool histVerificator::verify(const std::vector<histInfo>& histInfos, const std::string& fileName, profiler::outFormat frmt, std::string& error)
{
	std::ifstream outputFile = std::ifstream(fileName);
	if (!outputFile)
	{
		error = "FAILED to open " + fileName;
		return false;
	}
	
	std::unordered_map<std::string, histInfo> foundInfos;
	foundInfos.reserve(histInfos.size());

	switch(frmt)
	{
		case profiler::outFormat::excel:
			parseExcelFormat(outputFile, foundInfos);
		break;
		case profiler::outFormat::follow:
			parseFollowFormat(outputFile, foundInfos);
		break;
		default:
			error = "unsupported file format";
			return false;
		break;
	};
	
	if (histInfos.size() != foundInfos.size())
	{
		std::ostringstream stringStream;
		stringStream << "number of found histograms(" << foundInfos.size() << ") does not match the number of expected histograms " << histInfos.size();
		error = stringStream.str();
		return false;
	}

	for (size_t i = 0; i < histInfos.size(); i++)
	{
		const auto iter = foundInfos.find(histInfos[i].label);
		if (iter == foundInfos.end())
		{
			std::ostringstream stringStream;
			stringStream << "histogram " << histInfos[i].label << " was not found";
			error = stringStream.str();
			return false;
		}
		if (iter->second.numEntries != histInfos[i].numEntries)
		{
			std::ostringstream stringStream;
			stringStream << "histogram " << histInfos[i].label << " has unexpected number of entries " << iter->second.numEntries << " , expected " << histInfos[i].numEntries;
			error = stringStream.str();
			return false;
		}

		std::cout << "hist " << iter->second.label << " is ok" << std::endl;
	}

	return true;
}

std::string getHeaderAttribute(const std::string& line, size_t index, char separator)
{
	int i = 0;
	std::stringstream ssLine{ line };
	for (std::string attribute; std::getline(ssLine, attribute, separator); )
	{
		if (i == index)
			return attribute;
		++i;
	}
	return std::string();
}
std::string getHeaderAttribute(const std::string& line, const std::string& name, char separator)
{
	std::stringstream ssLine{ line };
	for (std::string attribute; std::getline(ssLine, attribute, separator); )
	{
		if (attribute.size() >= name.size() && name.compare(0, name.size() - 1, attribute, 0, name.size() - 1) == 0)
			return attribute;

		int res= name.compare(0, name.size() - 1, attribute, 0, name.size() - 1);
		res = res;
	}
	return std::string();
}
std::string getHeaderAttributeValue(const std::string& line, const std::string& attributeName, char separator)
{
	std::string attribute = getHeaderAttribute(line, attributeName, separatorFields);
	std::stringstream ssLine{ attribute };
	std::string val;
	std::getline(ssLine, val, separator); // key, attributeName
	std::getline(ssLine, val, separator); // the value
	return val;
}
void parseExcelFormat(std::ifstream& outputFile, std::unordered_map<std::string, histVerificator::histInfo>& foundInfos)
{
	std::string line;
	

	std::vector<std::unordered_map<std::string, histVerificator::histInfo>::iterator> indexes;

	// parse first line: labels and hist info
	std::getline(outputFile, line);
	std::stringstream ssLine{line};
	for (std::string part; std::getline(ssLine, part, separatorHist); )
	{
		// first field is label
		std::string name = getHeaderAttribute(part, 0, separatorFields);
		if (name.empty())
		{
			std::cerr << "empty hist name: " << part << std::endl;
			continue;
		}

		size_t overflows{ 0 };
		common::stoT(getHeaderAttributeValue(part, " #overflows", separatorAttribute), overflows);
		size_t underflows{ 0 };
		common::stoT(getHeaderAttributeValue(part, " #underflows", separatorAttribute), underflows);

		foundInfos[name] = histVerificator::histInfo{ name , overflows + underflows };
		indexes.push_back(foundInfos.find(name));
	}

	/* parse data, skip the last line,
	164	0	
	193	0	
	186	0	
	123	0	
	*/
	if (!std::getline(outputFile, line))
		return;
	std::string nextLine;
	while (std::getline(outputFile, nextLine))
	{
		if (nextLine.empty())
			continue;

		size_t i{0};
		std::stringstream ssLine{ line };
		for (std::string part; std::getline(ssLine, part, separatorHist); )
		{
			if (i >= indexes.size())
			{
				std::cerr << "unexpected number of fields, indexes.size: " << indexes.size() << std::endl;
				break;
			}
			if (part.empty())
				continue;

			size_t num = std::atoi(part.c_str());
			indexes[i]->second.numEntries += num;
			i++;
		}

		line = std::move(nextLine);
	}
}
void parseFollowFormat(std::ifstream& outputFile, std::unordered_map<std::string, histVerificator::histInfo>& foundInfos)
{

}
