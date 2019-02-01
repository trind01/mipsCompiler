//clang++ -g -Wall -Wextra mipsCompiler.cpp -std=c++11
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
using namespace std;


void parseLine(string line,string *arrParsed);
string constructRType(string opcode,string funct,string mips[],int i);
string constructIType(string opcode,string mips[],int i,int lineNumber);
string constructJType(string opcode,string mips[],int i,int lineNumber);
bool is_hex(string num);
bool is_dec(string num);
string hexToBinary(string num);
string decToBinary(string num);
bool sepOffReg(string* offset, string* reg, string s);
string padImm(string imm,int length);
string twosComp(string bin);

struct Instruction{
	string opcode;
	char type;
	string funct;
};

struct missingLabel{
	int instructionLine;
	string Label;
	char type;
};

const unordered_map<string,Instruction> ISA = {
	{"add",{"000000",'r',"100000"}},
	{"addi",{"001000",'i',"XXXXXX"}},
	{"addiu",{"001001",'i',"XXXXXX"}},
	{"addu",{"000000",'r',"100001"}},
	{"and",{"000000",'r',"100100"}},
	{"andi",{"001100",'i',"XXXXXX"}},
	{"beq",{"000100",'i',"XXXXXX"}},
	{"bne",{"000101",'i',"XXXXXX"}},
	{"div",{"000000",'r',"011010"}},
	{"divu",{"000000",'r',"011011"}},
	{"j",{"000010",'j',"XXXXXX"}},
	{"jal",{"000011",'j',"XXXXXX"}},
	{"jr",{"000000",'r',"001000"}},
	{"lbu",{"100100",'i',"XXXXXX"}},
	{"lhu",{"100101",'i',"XXXXXX"}},
	{"lui",{"001111",'i',"XXXXXX"}},
	{"lw",{"100011",'i',"XXXXXX"}},
	{"mfhi",{"000000",'r',"010000"}},
	{"mflo",{"000000",'r',"010010"}},
	{"mfc0",{"010000",'r',"XXXXXX"}},
	{"mult",{"000000",'r',"011000"}},
	{"multu",{"000000",'r',"011001"}},
	{"nor",{"000000",'r',"100111"}},
	{"xor",{"000000",'r',"100110"}},
	{"or",{"100101",'r',"100101"}},
	{"ori",{"001101",'i',"XXXXXX"}},
	{"sb",{"101000",'i',"XXXXXX"}},
	{"sh",{"101001",'i',"XXXXXX"}},
	{"slt",{"000000",'r',"101010"}},
	{"slti",{"001010",'i',"XXXXXX"}},
	{"sltiu",{"001011",'i',"XXXXXX"}},
	{"sltu",{"000000",'r',"101011"}},
	{"sll",{"000000",'r',"000000"}},
	{"sllv",{"000000",'r',"000100"}},
	{"srl",{"000000",'r',"000010"}},
	{"srlv",{"000000",'r',"000110"}},
	{"sra",{"000000",'r',"000011"}},
	{"srav",{"000000",'r',"000111"}},
	{"sub",{"000000",'r',"100010"}},
	{"subu",{"000000",'r',"100011"}},
	{"sw",{"101011",'i',"XXXXXX"}},
	{"nop",{"000000",'n',"XXXXXX"}}
};

const unordered_map<string,string> RegisterFile = {
	{"$0","00000"},
	{"$zero","00000"},
	{"$at","00001"},
	{"$v0","00010"},
	{"$v1","00011"},
	{"$a0","00100"},
	{"$a1","00101"},
	{"$a2","00110"},
	{"$a3","00111"},
	{"$t0","01000"},
	{"$t1","01001"},
	{"$t2","01010"},
	{"$t3","01011"},
	{"$t4","01100"},
	{"$t5","01101"},
	{"$t6","01110"},
	{"$t7","01111"},
	{"$s0","10000"},
	{"$s1","10001"},
	{"$s2","10010"},
	{"$s3","10011"},
	{"$s4","10100"},
	{"$s5","10101"},
	{"$s6","10110"},
	{"$s7","10111"},
	{"$t8","11000"},
	{"$t9","11001"},
	{"$k0","11010"},
	{"$k1","11011"},
	{"$gp","11100"},
	{"$sp","11101"},
	{"$fp","11110"},
	{"$ra","11111"},
};

const unordered_map<string,bool> rShift = {
	{"sll",true},
	{"sllv",false},
	{"srl",true},
	{"srlv",false},
	{"sra",true},
	{"srav",false}
};

const unordered_map<string,bool> iOffset = {
	{"beq",false},
	{"bne",false},
	{"lbu",true},
	{"lhu",true},
	{"lui",true},
	{"lw",true},
	{"sb",true},
	{"sh",true},
	{"sw",true}
};

const unordered_map<char,string> hexBinary = {
	{'0',"0000"},
	{'1',"0001"},
	{'2',"0010"},
	{'3',"0011"},
	{'4',"0100"},
	{'5',"0101"},
	{'6',"0110"},
	{'7',"0111"},
	{'8',"1000"},
	{'9',"1001"},
	{'A',"1010"},
	{'B',"1011"},
	{'C',"1100"},
	{'D',"1101"},
	{'E',"1110"},
	{'F',"1111"}
};

unordered_map<string,int> Labels;
vector<missingLabel> unsolvedIs;

int main(int argc, char* argv[]){
	vector<string> encodedIs;
	string line;
	int lineNumber = 0;
	ifstream mipsFile (argv[1]);
	if(mipsFile.is_open())
	{
		while (getline(mipsFile,line))
		{
			string arrInstrution[5];
			//Parse the String
			parseLine(line,arrInstrution);
			//Check if Instruction is in ISA
			auto got = ISA.find(arrInstrution[0]);
			if(got == ISA.end()){	//Not in ISA
				//Check if its a Label
				if(arrInstrution[0][arrInstrution[0].length()-1] == ':'){
					string lab = arrInstrution[0].substr(0,arrInstrution[0].length()-1);
					Labels.insert({lab,lineNumber});
					got = ISA.find(arrInstrution[1]); //Get Instruction
					if(got != ISA.end()){ //In ISA
						if(got->second.type == 'r'){
							encodedIs.push_back(constructRType(got->second.opcode,
								got->second.funct,arrInstrution,1));
						} else if(got->second.type == 'i'){
							encodedIs.push_back(constructIType(got->second.opcode,
								arrInstrution,1,lineNumber));
						} else if(got->second.type == 'j'){
							//construnctJType
							encodedIs.push_back(constructJType(got->second.opcode,
								arrInstrution,1,lineNumber));
						} else if(got->second.type == 'n'){
							encodedIs.push_back("00000000000000000000000000000000");
						} else{ encodedIs.push_back("INCORRECT FORMAT for: "+line);}
					} else{ encodedIs.push_back("INCORRECT FORMAT for: "+line);}
				} else{ encodedIs.push_back("INCORRECT FORMAT for: "+line);}
			}
			else{ //In ISA
				if(got->second.type == 'r'){
					encodedIs.push_back(constructRType(got->second.opcode,
						got->second.funct,arrInstrution,0));
				} else if(got->second.type == 'i'){
					encodedIs.push_back(constructIType(got->second.opcode,
						arrInstrution,0,lineNumber));
				} else if(got->second.type == 'j'){
					//construnctJType
					encodedIs.push_back(constructJType(got->second.opcode,
						arrInstrution,0,lineNumber));
				} else if(got->second.type == 'n'){
					encodedIs.push_back("00000000000000000000000000000000");
				} else{encodedIs.push_back("INCORRECT FORMAT for: "+line);}
			}
			lineNumber++;
		}
	}
	mipsFile.close();

	for(int i=0;i<(int)unsolvedIs.size();i++){
		int offset,pad = 0;
		string imm;
		auto got = Labels.find(unsolvedIs[i].Label);
		if(got == Labels.end()){cout << "INCORRECT FORMAT" << endl;}
		else{
			int numLine = unsolvedIs[i].instructionLine;
			if(unsolvedIs[i].type == 'j'){
				pad = 26;
				offset = got->second*4;
				imm = "0000"+decToBinary(to_string(offset));
				imm.erase(imm.length()-2,imm.length()-1);
			}
			else{
				pad = 16;
				offset = got->second-numLine-1;
				imm = decToBinary(to_string(offset));
			}			
			imm = padImm(imm,pad);
			encodedIs[numLine] = encodedIs[numLine] + imm;
		}
	}

	//output file
	ofstream outputFile;
	outputFile.open("imemContents.txt");
	for(int i = 0; i<(int)encodedIs.size();i++){
		outputFile << encodedIs[i] << endl;
	}
	outputFile.close();
	return 0;
}

void parseLine(string line,string *arrParsed)
{
	size_t pos=0;
	int arrI = 0;
	while( (pos = line.find(" ")) != std::string::npos)
	{
		string s = line.substr(0,pos);
		if(s[s.length()-1] == ','){s.erase(s.length()-1,s.length()-1);}
		arrParsed[arrI] = s;
		arrI++;
		line.erase(0,pos + 1);
	}
	if(line[line.length()-1] == '\n' || line[line.length()-1] == '\r'){
		line.erase(line.length()-1,line.length()-1);
		
	}
	arrParsed[arrI] = line;
}

string constructRType(string opcode,string funct,string mips[],int i)
{
	string sreg,treg,dreg,reg[3],encodedI,shiftNum;
	auto shift = rShift.find(mips[i]);
	i++;
	for(int j = i; j < i + 3; j++){
		auto got = RegisterFile.find(mips[j]);
		if(got == RegisterFile.end()){
			return "INCORRECT FORMAT";}
		else{
			reg[j-i] = got->second;
		}
	}
	if(shift != rShift.end()){ //Check if Shift type
		if(shift->second){
			//Construct Immediate Shift
			dreg = reg[0];
			treg = reg[1];
			if(is_hex(reg[2])){shiftNum = hexToBinary(reg[2]);}
			else{shiftNum = decToBinary(reg[2]);}
			padImm(shiftNum,5);
			encodedI = opcode + "00000" + treg + dreg + shiftNum + funct;
		} else{
			//Construct Register Variable Shift
			dreg = reg[0];
			treg = reg[1];
			sreg = reg[2];
			encodedI = opcode + sreg + treg + dreg + "00000" + funct;
		}
	}
	else{
		sreg = reg[1];
		treg = reg[2];
		dreg = reg[0];
		encodedI = opcode + sreg + treg + dreg + "00000" + funct;	
	}
	return encodedI;
}

string constructIType(string opcode,string mips[],int i,int lineNumber)
{
	string treg,sreg,encodedI;
	string imm = "";
	auto offset = iOffset.find(mips[i]);
	i++;
	if(offset != iOffset.end()){
		if(!offset->second){
			//Need Label or offset//Instruction $s,$t,offset
			auto got = RegisterFile.find(mips[i]);
			if(got == RegisterFile.end()){return "INCORRECT FORMAT";}
			else{sreg = got->second; i++;}
			got = RegisterFile.find(mips[i]);
			if(got == RegisterFile.end()){return "INCORRECT FORMAT";}
			else{treg = got->second; i++;}
			string offsetMips = mips[i];
			if(is_hex(mips[i])){
				string hex_num = mips[i].substr(2,mips[i].length()-3);
				imm = hexToBinary(hex_num);
				imm = padImm(imm,16);
			} else if(is_dec(mips[i])){
				imm = decToBinary(mips[i]);
				imm = padImm(imm,16);
			} else{
				auto Label = Labels.find(mips[i]);
				if(Label == Labels.end()){ //Label Missing or not reached yet
					unsolvedIs.push_back({lineNumber,mips[i],'i'});
				} else{
					int offsetNum = Label->second - lineNumber - 1;
					imm = decToBinary(to_string(offsetNum));
					imm = padImm(imm,16);
				}
			}
		}else{ //Instruction $t, offset($s) or $t imm
			auto got = RegisterFile.find(mips[i]);
			if(got == RegisterFile.end()){return "INCORRECT FORMAT";}
			else{treg = got->second; i++;}
			//separate offset from reg
			if(!sepOffReg(&imm,&sreg,mips[i])){return "INCORRECT FORMAT";}
		}
	} else{ //Instruction $t,$s,imm
			auto got = RegisterFile.find(mips[i]);
			if(got == RegisterFile.end()){return "INCORRECT FORMAT";}
			else{treg = got->second; i++;}
			got = RegisterFile.find(mips[i]);
			if(got == RegisterFile.end()){return "INCORRECT FORMAT";}
			else{sreg = got->second; i++;}
			if(is_hex(mips[i])){imm = hexToBinary(mips[i]); imm = padImm(imm,16);}
			else if(is_dec(mips[i])){imm = decToBinary(mips[i]); imm = padImm(imm,16);}
			else{return "INCORRECT FORMAT";}
	}
	encodedI = opcode + sreg + treg + imm;
	return encodedI;
}

string constructJType(string opcode,string mips[],int i,int lineNumber)
{
	string encodedI,imm = "";
	string target = mips[i+1];
	if(is_hex(target)){
		imm = hexToBinary(target);
		imm = padImm(imm,26);
	}else if(is_dec(target)){
		imm = decToBinary(target);
		imm = padImm(imm,26);
	}else{
		auto got = Labels.find(target);
		if(got == Labels.end()){
			unsolvedIs.push_back({lineNumber,target,'j'});
		} else{
			int offset = got->second*4;
			imm = "0000" + decToBinary(to_string(offset));
			imm.erase(imm.length()-2,imm.length()-1);
			imm = padImm(imm,26);
		}
	}
	encodedI = opcode + imm;
	return encodedI;
}

bool is_hex(string numI){
	int i = 0;
	bool neg = false;
	string num = numI;
	if(num[i] == '-'){neg = true;i++;}
	if(num[i] == '0' && num[i+1] == 'x'){
		if(neg){num.erase(0,3);}else{num.erase(0,2);}
		return num.find_first_not_of("0123456789ABCDEFabcdef") == std::string::npos;
	}
	return false;
}

bool is_dec(string numI){
	string num = numI;
	if(num[0] == '-'){num.erase(0,1);}
	return num.find_first_not_of("0123456789") == std::string::npos;
}

string hexToBinary(string num){
	string hex = "";
	bool neg = false;
	if(num[0] == '-'){neg = true; num.erase(0,3);}
	else{num.erase(0,2);}
	for(int i=0;i<(int)num.length();i++){
		char c = num[i];
		hex += hexBinary.find(toupper(c))->second;
	}
	if(neg){hex = "1" + twosComp(hex);}
	else{hex = "0" + hex;}
	return hex;
}

string decToBinary(string num){
	bool neg = false;
	if(num[0] == '-'){neg = true; num.erase(0,1);}
	int decNum = std::atoi(num.c_str());
	string dec = "";
	while(decNum >= 1){
		if((decNum%2)){dec = "1" + dec;}
		else{dec = "0" + dec;}
		decNum = decNum/2;
	}
	if(!neg){dec = "0"+dec;}
	else{dec = "1" + twosComp(dec);}
	return dec;
}

bool sepOffReg(string* offset, string* reg, string s)
{
	size_t pos=0;
	pos = s.find("(");
	if(pos == std::string::npos){
		if(is_hex(s)){
			*offset = hexToBinary(s);
			*offset = padImm(*offset,16);
			*reg = "00000";
			return true;
		} else if(is_dec(s)){
			*offset = decToBinary(s);
			*offset = padImm(*offset,16);
			*reg = "00000";
			return true;
		}
		return false;
	}
	string o = s.substr(0,pos);
	if(is_hex(o)){*offset = hexToBinary(o);}
	else if(is_dec(o)){*offset = decToBinary(o);}
	else{return false;}
	*offset = padImm(*offset,16);
	size_t pos2 = s.find(")");
	*reg = s.substr(pos+1,pos2-pos-1);
	auto got = RegisterFile.find(*reg);
	*reg = got->second;
	return true;
}

string padImm(string imm,int length)
{
	int extraLength = length - imm.length();
	bool neg = (imm[0] == '1');
	if(extraLength < 0){
		while(extraLength != 0){
			imm.erase(0,1);
			extraLength++;
		}
	}
	while(extraLength !=0){
		if(neg){imm = "1" + imm;}
		else{imm = "0" + imm;}
		extraLength--;
	}
	return imm;
}

string twosComp(string bin)
{
	int last1 = bin.length()-1;
	for(int i=last1;i>-1;i--){
		if(bin[i] == '1'){
			last1 = i;
			break;
		}
	}
	for(int i=0;i<last1;i++){
		if(bin[i] == '0'){bin[i] = '1';}
		else{bin[i] = '0';}
	}
	return bin;
}