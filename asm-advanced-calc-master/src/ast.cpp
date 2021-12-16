#include "ast.h"
#include <iostream>
#include <sstream>
#include <set>
#include "asm.h"

const char * floatTemps[] = {"$f0",
                            "$f1",
                            "$f2",
                            "$f3",
                            "$f4",
                            "$f5",
                            "$f6",
                            "$f7",
                            "$f8",
                            "$f9",
                            "$f10",
                            "$f11",
                            "$f12",
                            "$f13",
                            "$f14",
                            "$f15",
                            "$f16",
                            "$f17",
                            "$f18",
                            "$f19",
                            "$f20",
                            "$f21",
                            "$f22",
                            "$f23",
                            "$f24",
                            "$f25",
                            "$f26",
                            "$f27",
                            "$f28",
                            "$f29",
                            "$f30",
                            "$f31"
                        };

#define FLOAT_TEMP_COUNT 32
set<string> intTempMap;
set<string> floatTempMap;

extern Asm assemblyFile;

int globalStackPointer = 0;

string getFloatTemp(){
    for (int i = 0; i < FLOAT_TEMP_COUNT; i++)
    {
        if(floatTempMap.find(floatTemps[i]) == floatTempMap.end()){
            floatTempMap.insert(floatTemps[i]);
            return string(floatTemps[i]);
        }
    }
    cout<<"No more float registers!"<<endl;
    return "";
}

void releaseFloatTemp(string temp){
    floatTempMap.erase(temp);
}

void FloatExpr::genCode(Code &code){
    string floatTemp = getFloatTemp();
    code.place = floatTemp;
    stringstream ss;
    ss<< "li.s"<<floatTemp<<", "<< this->number << endl;
    code.code = ss.str();
}

void SubExpr::genCode(Code &code){
     Code leftcode;
    Code rightcode;
    this->expr1->genCode(leftcode);
    this->expr2->genCode(rightcode);
    stringstream ss; 
    code.place = getFloatTemp();
    ss<< "sub.s"<< code.place<< ", "<< leftcode.place << ", " << rightcode.place<<endl;
    releaseFloatTemp(leftcode.place);
    releaseFloatTemp(rightcode.place);
    code.code = ss.str();
}

void DivExpr::genCode(Code &code){
    Code leftcode;
    Code rightcode;
    this->expr1->genCode(leftcode);
    this->expr2->genCode(rightcode);
    stringstream ss; 
    code.place = getFloatTemp();
    ss<< "div.s"<< code.place<< ", "<< leftcode.place << ", " << rightcode.place<<endl;
    releaseFloatTemp(leftcode.place);
    releaseFloatTemp(rightcode.place);
    code.code = ss.str();
}

void IdExpr::genCode(Code &code){
    string floatTemp = getFloatTemp();
    code.place = floatTemp;
    code.code = "l.s "+ floatTemp + ", " + this->id + "\n";
}

string ExprStatement::genCode(){
    Code exprCode;
    this->expr->genCode(exprCode);
    releaseFloatTemp(exprCode.place);
    return exprCode.code;
}

string IfStatement::genCode(){
    string endIfLabel = "endif";
    Code exprCode;
    this->conditionalExpr->genCode(exprCode);
    stringstream code;
    code<<exprCode.code<<endl;
    code<<"bc1f "< endIfLabel<< endl; 
    code<<this->trueStatement.begin()<< endl
    << endIfLabel<< " :"<< endl;
    releaseFloatTemp(exprCode.place);

    return code.str();

}

void MethodInvocationExpr::genCode(Code &code){
    list<Expr *>::iterator it = this->expressions.begin();
    list<Code> codes;
    stringstream ss;
    Code argCode;
    while (it != this->expressions.end())
    {
        (*it)->genCode(argCode);
        ss << argCode.code <<endl;
        codes.push_back(argCode);
        it++;
    }

    int i = 0;
    list<Code>::iterator placesIt = codes.begin();
    while (placesIt != codes.end())
    {
        releaseFloatTemp((*placesIt).place);
            ss << "mfc1 $a"<<i<<", "<< (*placesIt).place<<endl;
        i++;
        placesIt++;
    }
    ss<< "jal "<< this->id<<endl;
    string reg;
    reg = getFloatTemp();
    ss << "mtc1 $v0, "<< reg<<endl;
    code.code = ss.str();
    code.place = reg;  
}

string AssignationStatement::genCode(){

}

void GteExpr::genCode(Code &code){
    Code leftSideCode;
    Code rightSideCode;
    stringstream ss;
    this->expr1->genCode(leftSideCode);
    this->expr2->genCode(rightSideCode);
    ss << leftSideCode.code << endl<< rightSideCode.code << endl;
    releaseFloatTemp(leftSideCode.place);
    releaseFloatTemp(rightSideCode.place);
    ss<< "c.gt.s"<< leftSideCode.place<< ", "<< rightSideCode.place<< endl;

    code.code = ss.str();
}

void LteExpr::genCode(Code &code){
    Code leftSideCode;
    Code rightSideCode;
    stringstream ss;
    this->expr1->genCode(leftSideCode);
    this->expr2->genCode(rightSideCode);
    ss << leftSideCode.code << endl<< rightSideCode.code << endl;
    releaseFloatTemp(leftSideCode.place);
    releaseFloatTemp(rightSideCode.place);
    ss<< "c.lt.s"<< leftSideCode.place<< ", "<< rightSideCode.place<< endl;

    code.code = ss.str();
}

void EqExpr::genCode(Code &code){
    Code leftSideCode; 
    Code rightSideCode;
    this->expr1->genCode(leftSideCode);
    this->expr2->genCode(rightSideCode);
    stringstream ss;
    releaseRegister(leftSideCode.place);
    releaseRegister(rightSideCode.place);
    ss << leftSideCode.code << endl
    << rightSideCode.code <<endl;
    code.code = ss.str();
}

void ReadFloatExpr::genCode(Code &code){
    Code ExprCode;
    stringstream ss;
    this->genCode(ExprCode);
    ss<< "li $t0 " << ExprCode.code << end;
    releaseFloatTemp(ExprCode.place);
    code.code = ss.str();

}

string PrintStatement::genCode(){
    int i =0 ;
    stringstream code;
    code<< "la $a0, "<< this->id <<endl
    <<"li $v0, 4"<< endl
    <<"syscall"<<endl;
    code<<"mov.s $f12, "  <<endl
    <<"li $v0, 2"<< endl
    <<"syscall"<<endl;
    

    return code.str();
}

string ReturnStatement::genCode(){
    Code exprCode;
    this->expr->genCode(exprCode);
    releaseFloatTemp(exprCode.place);
    stringstream ss;
    ss<<exprCode.code<< endl;
    ss<<"mfc1 $v0, "<<exprCode.place<< endl;
    return ss.str();
}

string MethodDefinitionStatement::genCode(){
    if(this->stmts.empty())
        return "";
    int stackPointer = 4; 
    globalStackPointer = 0;
    stringstream code; 
    code << this->id<<": "<< endl;
    if(this->params.size() > 0){
        list<Parameter *>:: iterator it = this->params.begin();
        for(int i = 0; i< this->params.size(); i++ ){
            code << "sw $a"<<i<<", "<< stackPointer<<"($sp)"<<endl;
            stackPointer +=4;
            globalStackPointer +=4;
            it++;
        }
    }
    code<< this->stmts.begin()<<endl;
    stringstream sp;
    int currentStackPointer = globalStackPointer;
    sp<<endl<<" addiu $sp, $sp, -"<<currentStackPointer<<endl;
    code<<"addiu $sp, $sp, "<<currentStackPointer<<endl;
    code <<"jr $ra"<<endl;
    string result = code.str();
    result.insert(id.size() + 2, sp.str());
    return result;
}