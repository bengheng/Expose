#include "tinyxml.h"
#include <ida.hpp>
#include <idp.hpp>
#include <funcs.hpp>
#include <list>
#include "function_analyzer.h"

using namespace std;

void GetOpcodesMnem(
	TiXmlElement *pBBEle,
	const ea_t node_ea,
	function_analyzer &F,
	char buffer[2048],
	size_t bufflen)
{
	ea_t curr_ea = node_ea;
	ea_t curr_instr_ea;
	int count = 0;

	buffer[0] = 0;
	// While address is in node...
	while(curr_ea != BADADDR &&
	node_ea == F.get_node(curr_ea) &&
	count < (bufflen-1))	
	{
		ea_t next_ea = F.next_ea(curr_ea);
	
		// Keep checking the next byte until it is not code,
		// or insufficient buffer, or we reached the address
		// of the next instruction.	
		while(isCode(get_flags_novalue(curr_ea)) &&
		count < (bufflen-1) &&
		curr_ea != next_ea)
		{
			char value[3] = {0};
			qsnprintf(value, 3, "%02x", get_byte(curr_ea));

			// 1 byte takes up 2 buffer cells
			buffer[count] = value[0]; count++;
			buffer[count] = value[1]; count++;
			curr_ea++;
		}
		
		curr_ea = next_ea;
	}

	buffer[count] = '\0'; // end with NULL
}

void WriteBasicBlocks(
	TiXmlElement *pFuncEle,
	function_analyzer &F,
	list<TiXmlElement*> &element_list,
	list<TiXmlText*> &text_list)
{
	//msg("Writing basicblocks...\n");

	char buffer[2047];
	for(int n = 1; n <= F.get_num_nodes(); ++n)
	{
		ea_t node_ea = F.get_node(n);
		TiXmlElement *pBBEle = new TiXmlElement("BB");
		pBBEle->SetAttribute( "ea", node_ea);

		GetOpcodesMnem(pBBEle, node_ea, F, buffer, 2046);

		TiXmlText *pText = new TiXmlText(buffer);	
		pBBEle->LinkEndChild(pText);
		text_list.push_back(pText);

		pFuncEle->LinkEndChild(pBBEle);
		element_list.push_back(pBBEle);
	}
}

void WriteFuncs(
	TiXmlElement *pRoot,
	list<TiXmlElement*> &element_list,
	list<TiXmlText*> &text_list)
{

	for(int f = 0; f < get_func_qty(); ++f)
	{
		func_t *curFunc = getn_func(f);
		function_analyzer F(curFunc->startEA);
		F.run_analysis();

		// Add new function element
		TiXmlElement *pFuncEle = new TiXmlElement("Function");
		pFuncEle->SetAttribute("name", F.get_name());

		WriteBasicBlocks(pFuncEle, F, element_list, text_list);
		
		pRoot->LinkEndChild(pFuncEle);
		element_list.push_back(pFuncEle);
	}
}

void DumpMnem()
{
	list<TiXmlElement*> element_list;
	list<TiXmlText*> text_list;
	
	TiXmlDocument doc;
	TiXmlDeclaration * decl = new TiXmlDeclaration("1.0", "", "");
	doc.LinkEndChild(decl);

	// Gets input file path
	char inpath[1024];
	get_input_file_path(inpath, sizeof(inpath));

	// Creates root element and set file path as attribute
	TiXmlElement *root = new TiXmlElement("file");
	root->SetAttribute("filepath", inpath);
	
	//TiXmlText *text = new TiXmlText("World");
	//element->LinkEndChild(text);
	
	WriteFuncs(root, element_list, text_list);
	doc.LinkEndChild(root);

	char outpath[1024];
	char *outname;
	qsnprintf(outpath, sizeof(outpath)-1, "%s.xml", inpath);
	for(int n = qstrlen(outpath)-1; n >= 0; --n)
	{
		if(outpath[n] == '/')
			outname = &outpath[n];
	}
	doc.SaveFile(outname);

	text_list.clear();
	element_list.clear();
}
