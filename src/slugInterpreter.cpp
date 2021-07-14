#include "slugInterpreter.h"
#include <iostream>
//#include <ctype.h>
#include <cmath>

/***** Standard functions *****/
bool Interpreter::inInts(std::string potentialInt){
    return integers.find(potentialInt) != integers.end();
}

bool Interpreter::inFloats(std::string potentialFloat){
    return floats.find(potentialFloat) != floats.end();
}

bool Interpreter::inStrings(std::string potentialStr){
    return strings.find(potentialStr) != strings.end();
}

bool Interpreter::inBools(std::string potentialBool){
    return booleans.find(potentialBool) != booleans.end();
}

std::string takeOffFrontChar(std::string str){
    return str.substr(1, str.length() - 1);
}

int getPrecedence(char op){
    if (op == '+' || op == '-'){
        return 1;
    } else if (op == '*' || op == '/'){
        return 2;
    } else if (op == '%'){
        return 3;
    } else if (op == '^'){
        return 4;
    } else {
        return -1;
    }
}


void operateOnAns(float * ans, char op, float num, Interpreter * interp){
    switch (op){
        case '+': // Addition
            *ans += num;
            break;
        case '-': // Subtraction
            *ans -= num;
            break;
        case '*': // Multiplication
            *ans *= num;
            break;
        case '/': // Division
            *ans /= num;
            break;
        case '%': // Modulus
            *ans = (int)(*ans) % (int)num;
            break;
        case '^':
            *ans = powf(*ans, num);
            break;
        default:
            std::cout << "OP: " << op << std::endl;
            break;
    }
}

bool isOp(char op){
    return op == '+' || op == '-' || op == '*' || op == '/' || op == '%' || op == '^';
}

class NumberStack{
    private:
        std::vector<float> stack;
    public:
        void push(std::string val, Interpreter * interp);
        void push(float val){
            stack.push_back(val);
        }
        float pop(){
            float temp = stack.back();
            stack.pop_back();
            return temp;
        }
};

std::vector<std::string> tinyTokenizer(std::string s){
    bool alphanum = true;
    bool parens = false;
    int parensNum = 0;
    std::vector<std::string> tokens;
    std::string current_token = "";
    s = s.substr(1, s.size() - 2); // Removing outer parentheses
    std::string truStr = "";
    for (int i = 0; i < s.size(); i++){if(s[i] != ' '){truStr+=s[i];}} // Removing spaces
    for (int i = 0; i < truStr.size(); i++){
        if ((isalnum(truStr[i]) || truStr[i] == '.') && !parens){// var or number literal
            if (!alphanum){ // Token changed
                alphanum = true;
                tokens.push_back(current_token);
                current_token = "";
            }
            current_token += truStr[i];
        } else if (isOp(truStr[i]) && !parens){ // operator or parentheses
            if (alphanum){
                alphanum = false;
                tokens.push_back(current_token);
                current_token = "";
            }
            current_token += truStr[i];
        } else if (truStr[i] == '('){ // Start parentheses
            if (parensNum == 0){
                tokens.push_back(current_token);
                current_token = "(";
            }
            parensNum++;
            parens = true;
        } else if (truStr[i] == ')'){ // End parentheses
            parensNum--;
            current_token += truStr[i];
            if (parensNum == 0){
                parens = false;
                tokens.push_back(current_token);
                current_token = "";
            }
        } else if (parens){
            current_token += truStr[i];
        }
    }
    if (current_token != ""){
        tokens.push_back(current_token);
    }
    return tokens;
}

// Getting the numerical value of whatever's been passed in
float evalNum(std::string num, Interpreter * interp){
    if (!isdigit(num[0]) && num[0] != '-' && num[0] != '('){ // not a number literal and not a negative number literal
        if (interp->inInts(num)){
            return interp->integers[num];
        } else if (interp->inFloats(num)){
            return interp->floats[num];
        }
    } else if (num[0] == '('){ // Expression
        // Shunting yard algorithm
        // https://en.wikipedia.org/wiki/Shunting-yard_algorithm
        NumberStack s;
        auto tokens = tinyTokenizer(num);
        std::vector<std::string> polishTokens;
        // Convert to reverse polish
        for (std::vector<std::string>::size_type i = 0; i < tokens.size(); i++){ // Looping through tokens
            if (isOp(tokens[i][0])){ // Operator
                auto temp = tokens[i]; // Store the operator
                i++;
                polishTokens.push_back(tokens[i]); // The value
                polishTokens.push_back(temp); // The operator
            } else {
                polishTokens.push_back(tokens[i]);
            }
        }

        for (auto t : polishTokens){ // Looping through
            if (isOp(t[0])){
                float a = s.pop();
                float b = s.pop();
                operateOnAns(&b, t[0], a, interp);
                s.push(b);
            } else {
                s.push(t, interp);
            }
        }
        return s.pop();
    } else { // Is a number literal
        return std::stof(num, nullptr);
    }
    return 0;
}

void NumberStack::push(std::string val, Interpreter * interp){
    stack.push_back(evalNum(val, interp));
}

// Getting the string value of whatever has been passed in
std::string getStrValOf(std::string val, Interpreter * interp){
    if (interp->inInts(val)){ // integer var
        return std::to_string(interp->integers[val]);
    } else if (interp->inFloats(val)){ // float var
        return std::to_string(interp->floats[val]);
    } else if (interp->inStrings(val)){ // string var
        return interp->strings[val];
    } else if (isdigit(val[0]) || val[0] == '-'){ // Number literal
        return val;
    } else if (val[0] == '('){ // Expression
        return std::to_string(evalNum(val, interp));
    } else if (val[0] == '"'){ // string literal
        return takeOffFrontChar(val);
    } else {
        interp->callError("Error converting '" + val + "' to string");
    }
}

bool evalBool(std::string * args, Interpreter * interp){
    if (interp->inBools(args[0])){
        return interp->booleans[args[0]];
    }
    if (args[0][0] == '"' || interp->inStrings(args[0])){ // Comparison of string literal or string var
        std::string a = getStrValOf(args[0], interp);
        std::string b = getStrValOf(args[2], interp);
        if (args[1] == "=="){
            return a == b;
        } else if (args[1] == "<"){
            return a < b;
        } else if (args[1] == ">"){
            return a > b;
        } else if (args[1] == "!="){
            return a != b;
        }
    } else if (/*Number Literal*/(args[0][0] == '-' || isdigit(args[0][0])) || /*A number var*/(interp->inInts(args[0]) || interp->inFloats(args[0]))){
        float a = evalNum(args[0], interp);
        float b = evalNum(args[2], interp);
        if (args[1] == "=="){
            return a == b;
        } else if (args[1] == "<"){
            return a < b;
        } else if (args[1] == ">"){
            return a > b;
        } else if (args[1] == "<="){
            return a <= b;
        } else if (args[1] == ">="){
            return a >= b;
        } else if (args[1] == "!="){
            return a != b;
        }
    }
}

bool getBooleanValOf(std::string * args, Interpreter * interp, int argc){
    bool temp = false;
    std::vector<bool> parts;
    bool currentPart;
    std::string andOr;
    for (int i = 0; i < argc; i++){
        if (args[i] == "and"){ // and operation
            andOr += "a";
        } else if (args[i] == "or"){ // or operation
            andOr += "o";
        } else if (interp->inBools(args[i])){
            parts.push_back(interp->booleans[args[i]]);
        } else { // Evaluating the boolean expression
            std::string sTemp[3] = {args[i]};
            i++;
            sTemp[1] = args[i];
            i++;
            sTemp[2] = args[i];
            parts.push_back(evalBool(sTemp, interp));
        }
    }
    if (parts.size() == 1){
        return parts[0];
    }
    temp = parts[0];
    for (int i = 1; i < parts.size(); i++){
        if (andOr[i - 1] == 'a'){ // and
            temp = temp && parts[i];
        } else if (andOr[i - 1] == 'o'){ // or
            temp = temp || parts[i];
        }
    }
    return temp;
}

// Printing a string literal or var to the string
void print(std::string * printString, Interpreter * interp){ // ptinting a string
    std::cout << getStrValOf(printString[0], interp);
}

// Printing a string literal or var to the string with a newline
void println(std::string * printString, Interpreter * interp){ // ptinting a string
    std::cout << getStrValOf(printString[0], interp) << std::endl;
}

// A print that takes in a varied amount of arguments.
void slugPrintf(std::string * args, Interpreter * interp){
    std::string strToBeOutputted;
    for (int i = 0; i < interp->argsPassedIn; i++){
        strToBeOutputted += getStrValOf(args[i], interp);
    }
    std::cout << strToBeOutputted << std::endl;
}

/* Declaring and printing data types */
// takes in two arguments, name of var and value to set it to
void declareInt(std::string * declaration, Interpreter * interp){
    interp->integers[declaration[0]] = evalNum(declaration[1], interp);
}

// takes in two arguments, name of var and value to set it to
void declareFloat(std::string * declaration, Interpreter * interp){
    interp->floats[declaration[0]] = evalNum(declaration[1], interp);
}

// takes in two arguments, name of var and value to set it to
void declareStr(std::string * declaration, Interpreter * interp){
    interp->strings[declaration[0]] = getStrValOf(declaration[1], interp);
}

// takes in five arguments
// 1: name of bool
// 2: first val to be evaluated. Ex: 20
// 3: boolean operator Ex: >, <, ==
// 4: second val to be evaluated Ex: 10
void declareBool(std::string * args, Interpreter * interp){
    std::string temp[32];
    for (int i = 1; i < interp->argsPassedIn; i++){ // Collecting everything but the variable name
        temp[i-1] = args[i];
        std::cout << temp[i-1] << std::endl;
    }
    interp->booleans[args[0]] = getBooleanValOf(temp, interp, interp->argsPassedIn - 1);
}

/* Getting user input */
// Getting an int from the user
void readInt(std::string * args, Interpreter * interp){
    std::cin >> interp->integers[args[0]];
}

// Getting a float from the user
void readFloat(std::string * args, Interpreter * interp){
    std::cin >> interp->floats[args[0]];
}

// Getting a string from the user
void readStr(std::string * args, Interpreter * interp){ // The only way I could get it top consistently work
    std::string inStr;
    while(std::getline(std::cin, inStr)){ 
        if (inStr != ""){break;}
    } // getting a full line to account for spaces
    interp->strings[args[0]] = inStr;
}

/* conditionals */
// Takes one argument, a boolean variable
void ifSlug(std::string * args, Interpreter * interp){
    if (getBooleanValOf(args, interp, interp->argsPassedIn)){
        interp->curlyBraceLevel[interp->curlyBraceNum + 1][0] = true;
        interp->curlyBraceLevel[interp->curlyBraceNum + 1][1] = true;
    } else {
        interp->curlyBraceLevel[interp->curlyBraceNum + 1][0] = false;
    }
}

void elseifSlug(std::string * args, Interpreter * interp){
    if (!interp->curlyBraceLevel[interp->curlyBraceNum + 1][1]){ // past if statement failed
        if (getBooleanValOf(args, interp, interp->argsPassedIn)){
            interp->curlyBraceLevel[interp->curlyBraceNum + 1][0] = true;
            interp->curlyBraceLevel[interp->curlyBraceNum + 1][1] = true;
        } else {
            interp->curlyBraceLevel[interp->curlyBraceNum + 1][0] = false;
        }
    } else {
        interp->curlyBraceLevel[interp->curlyBraceNum + 1][0] = false;
    }
}

void elseSlug(std::string * args, Interpreter * interp){
    if (!interp->curlyBraceLevel[interp->curlyBraceNum + 1][1]){ // past if statement failed
        interp->curlyBraceLevel[interp->curlyBraceNum + 1][0] = true;
    } else {
        interp->curlyBraceLevel[interp->curlyBraceNum + 1][0] = false;
    }
}

void test(std::string * args, Interpreter * interp){
    std::cout << "Test" << std::endl;
}

void slugSystem(std::string * args, Interpreter * interp){
    system(getStrValOf(args[0], interp).c_str());
}

/* String manipulation functions */
// Standard sting concatenation
void slugConcat(std::string * args, Interpreter * interp){
    std::string fullStr = interp->strings[args[0]];
    for (int i = 1; i < interp->argsPassedIn; i++){
        fullStr += getStrValOf(args[i], interp);
    }
    interp->strings[args[0]] = fullStr;
}

// Reversing the string passed in
void reverseStr(std::string * args, Interpreter * interp){
    std::string reversedStr;
    std::string trueStr = getStrValOf(args[0], interp);
    for (int i = trueStr.length() - 1; i >= 0; i--){
        reversedStr += trueStr[i];
    }
    interp->strings[args[0]] = reversedStr;
}

// Getting the length of a string
// First val is the string
// Second val id the int to store the result in
void strLength(std::string * args, Interpreter * interp){
    interp->integers[args[1]] = getStrValOf(args[0], interp).length();
}

void slugSubstr(std::string * args, Interpreter * interp){
    std::string strVal = getStrValOf(args[0], interp);
    std::string result;
    result = strVal.substr((int)evalNum(args[2], interp), (int)evalNum(args[3], interp) - (int)evalNum(args[2], interp) + 1);
    interp->strings[args[1]] = result;
}

// The function that performs mathematical operations
// does not do PEMDAS, but goes from left to right
// This includes the variable that it is going to in the assignment
void evalSet(std::string * args, Interpreter * interp){
    float ans = evalNum(args[0], interp); // Starting off with the var

    for (int i = 1; i < interp->argsPassedIn; i += 2){ // Looping through all of the 
        operateOnAns(&ans, args[i][0], evalNum(args[i + 1], interp), interp);
    }

    // Storing the answer in the variable
    if (interp->inInts(args[0])){ // If the var passed in is an integer variable
        interp->integers[args[0]] = (int) ans;
    } else { // must be float
        interp->floats[args[0]] = ans;
    }
}

// Starting off with the first number/variable, not the variable it will be assigned into.
void evalAssign(std::string * args, Interpreter * interp){
    float ans = evalNum(args[1], interp); 

    for (int i = 2; i < interp->argsPassedIn; i += 2){ // Looping through all of the 
        operateOnAns(&ans, args[i][0], evalNum(args[i + 1], interp), interp);
    }

    // Storing the answer in the variable
    if (interp->inInts(args[0])){ // If the var passed in is an integer variable
        interp->integers[args[0]] = (int) ans;
    } else { // must be float
        interp->floats[args[0]] = ans;
    }
}

// takes in one argument, a var to sqrt
void slugSQRT(std::string * args, Interpreter * interp){
    if (interp->inInts(args[0])){ // integer var passed in
        interp->integers[args[0]] = sqrt(interp->integers[args[0]]);
    } else { // float var passed in
        interp->floats[args[0]] = sqrt(interp->floats[args[0]]);
    }
}

// Easter egg, wont be included in the docs
void dispSlug(std::string * args, Interpreter * interp){
    std::cout << "            ______      (o)o)     " << std::endl;
    std::cout << "          _'      '_    ////      " << std::endl;
    std::cout << "        _'          '-.///        " << std::endl;
    std::cout << "     .-'               //         " << std::endl;
    std::cout << "     '---..____...---''           " << std::endl;
}

// Deleting a variable
void slugDelete(std::string * args, Interpreter * interp){
    if (interp->inInts(args[0])){
        interp->integers.erase(args[0]);
    } else if (interp->inFloats(args[0])){
        interp->floats.erase(args[0]);
    } else if (interp->inStrings(args[0])){
        interp->strings.erase(args[0]);
    } else if (interp->inBools(args[0])){
        interp->booleans.erase(args[0]);
    } else {
        interp->callError("Error deleting variable: '" + args[0] + "' does not exist");
    }
}

/* Goto Statements */
void slugPoint(std::string * args, Interpreter * interp){ // Setting where the goto goes
    interp->pointNums[args[0]] = interp->lineNum;
}

void slugGoto(std::string * args, Interpreter * interp){ // Actually going to the point
    interp->lineNum = interp->pointNums[args[0]];
}

/* Loops */
// Deprecated
void slugWhile(std::string * args, Interpreter * interp){
    if (getBooleanValOf(args, interp, interp->argsPassedIn)){
        interp->curlyBraceLevel[interp->curlyBraceNum + 1][0] = true; // the next tablevel will be processed
        interp->loopLevel[interp->curlyBraceNum] = interp->lineNum - 1;
    } else {
        interp->curlyBraceLevel[interp->curlyBraceNum + 1][0] = false;
    }
}

/* Interpreter Functions */
Interpreter::Interpreter(){ // whenever an interpreter is initiated
    // For negative argcounts, The number is the minimum amount of args that could be passed in
    curlyBraceLevel[0][0] = true; // When there are no curly braces
    /* standard functions */
    // print functions
    functions.push_back({"print", 1, &print}); // adding the print function
    functions.push_back({"println", 1, &println}); // adding the print function
    functions.push_back({"printf", -1, &slugPrintf}); // adding the printf function
    // Declaration functions
    functions.push_back({"int", 2, &declareInt}); // the int declaration.
    functions.push_back({"float", 2, &declareFloat}); // the float declaration
    functions.push_back({"string", 2, &declareStr}); // the float declaration
    functions.push_back({"bool", -2, &declareBool}); // the boolean declaration
    // Input functions
    functions.push_back({"readInt", 1, &readInt}); // reading an integer from the user
    functions.push_back({"readFloat", 1, &readFloat}); // reading a float from the user
    functions.push_back({"readStr", 1, &readStr}); // reading a string from the user
    /* Conditional functions */
    functions.push_back({"if", -1, &ifSlug}); // the if statement 
    functions.push_back({"elseif", -1, &elseifSlug}); // the else if statement
    functions.push_back({"else", 0, &elseSlug}); // the else statement
    // Mathematical functions
    functions.push_back({"evalSet", -3, &evalSet}); // This is for math that includes the variable it assigns to in the operations
    functions.push_back({"evalAssign", -4, &evalAssign}); // This is for math that does not include the number in the equation
    functions.push_back({"sqrt", 1, &slugSQRT}); // sqrts the variable passed in.
    // String operations
    functions.push_back({"concat", -2, &slugConcat}); // String concatenation
    functions.push_back({"reverseStr", 1, &reverseStr}); // Reversing the string passed in
    functions.push_back({"strLength", 2, &strLength}); // Getting the length of the string passed in
    functions.push_back({"substr", 4, &slugSubstr}); // substr of a string passed in
    // Goto statements
    functions.push_back({"point", 1, &slugPoint});
    functions.push_back({"goto", 1, &slugGoto});
    // Other stuff
    functions.push_back({"slug", 0, &dispSlug});
    functions.push_back({"system", 1, &slugSystem});
    functions.push_back({"delete", 1, &slugDelete});
}

bool Interpreter::inFunctions(std::string potentialFunc){
    int i = 0;
    for (Function fun : functions){
        if (potentialFunc == fun.name){
            argcountForFunc = fun.argc;
            funcNum = i;
            return true;
        }
        i++;
    }
    return false;
}

void Interpreter::callError(std::string errorMsg){
    std::cout << "Error on line: " << lineNum + 1 << std::endl;
    std::cout << fullFile[lineNum] << std::endl;
    std::cout << errorMsg << std::endl;
    exit(EXIT_FAILURE); // Stopping the program
}

std::vector<std::string> Interpreter::tokenizer(std::string passedInString){
    std::vector<std::string> tokens;
    std::string token;
    bool isString = false;
    bool charSeen;
    int tabLevel = 0;
    int spaceNum = 0;
    for (int i = 0; i < passedInString.length(); i++){ // looping through string
        if (passedInString[i] != ' ' && !isString){
            charSeen = true;
            if (passedInString[i] == '"'){ // string started
                token += '"';
                isString = true;
            } else if (passedInString[i] == '#' || passedInString[i] == '{'){ // Comments and ignored characters
                return tokens; // Stop Tokenization
            } else {
                token += passedInString[i];
            }
        } else if (isString){
            if (passedInString[i] != '"'){
                token += passedInString[i];
            } else {
                isString = false;
            }
        } else { // space
            spaceNum++;
            if (spaceNum == 4 && !charSeen){
                tabLevel++;
                spaceNum = 0;
            }
            if (token != "" && charSeen){
                tokens.push_back(token);
                token = "";
            }
        }
    }
    if (token != ""){
        tokens.push_back(token); // adding last token
    }
    curlyBraceNum = tabLevel;
    return tokens;
}
