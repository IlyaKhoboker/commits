#pragma once
#include <string>
#include <msclr\marshal_cppstd.h>
#include <regex>
namespace Project3 {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::IO;
	using namespace System::Collections::Generic;
	//a:=(V+III)*(IX-V)+(dsda+I)+(V+III)+Sss+(dsda+I)
	// a: = XV + dasd / V + III
	public ref struct Lexeme {
		String^ type;
		String^ value;

		Lexeme(String^ type, String^ value) {
			this->type = type;
			this->value = value;
		}
	};

	bool isRomanNumeral(System::String^ str) {
		std::string s_str = msclr::interop::marshal_as<std::string>(str);

		// Римские числа должны следовать определенному порядку
		std::regex pattern("^M{0,3}(CM|CD|D?C{0,3})(XC|XL|L?X{0,3})(IX|IV|V?I{0,3})$");

		return std::regex_match(s_str, pattern);
	}

	bool isIdentifier(String^ str) {
		if (!Char::IsLetter(str[0]) && str[0] != '_') { // Идентификаторы должны начинаться с буквы или символа '_'
			return false;
		}
		for (int i = 1; i < str->Length; ++i) {
			if (!Char::IsLetterOrDigit(str[i]) && str[i] != '_') { // Идентификаторы могут содержать только буквы, цифры и символ '_'
				return false;
			}
		}

		array<wchar_t>^ chars = { 'I', 'V', 'X' };

		if (str->IndexOfAny(chars) != -1 && !isRomanNumeral(str)) {
			return false;
		}

		return true;
	}



	public ref class MyForm : public System::Windows::Forms::Form
	{
	public:
		String^ selectedFilePath; // Для хранения пути к выбранному файлу
		List<Lexeme^>^ tokenize(String^ text) {
			List<Lexeme^>^ lexemes = gcnew List<Lexeme^>();
			String^ buffer;
			bool assignmentFlag = false;
			bool commentFlag = false;
			bool operatorFlag = false;
			int openBrackets = 0;

			for (int i = 0; i < text->Length; ++i) {
				String^ c = text->Substring(i, 1);

				if (c == "#") {
					commentFlag = true;
					continue;
				}

				else if (c == "\n" && commentFlag) {
					commentFlag = false;
					continue;
				}
				else if (commentFlag) {
					continue;
				}

				if (Char::IsLetter(c[0]) || c[0] == '_') {
					buffer += c;
					if (i + 1 == text->Length || (!Char::IsLetterOrDigit(text[i + 1]) && text[i + 1] != '_')) {
						if (isRomanNumeral(buffer)) {
							/*for each (char numeral in buffer) {
								lexemes->Add(gcnew Lexeme{ "Roman numeral", gcnew String(numeral, 1) });
							}
							*/
							lexemes->Add(gcnew Lexeme{ "Roman numeral", buffer });
						}
						else if (isIdentifier(buffer)) {
							lexemes->Add(gcnew Lexeme{ "Identifier", buffer });
						}
						else {
							Console::WriteLine("Invalid identifier or Roman numeral '" + buffer + "' at position " + i);
							return nullptr;
						}
						buffer = "";
						operatorFlag = false;
					}
				}
				else if (Char::IsDigit(c[0])) {
					buffer += c;
					if (i + 1 == text->Length || (!Char::IsLetterOrDigit(text[i + 1]) && text[i + 1] != '_')) {
						if (isIdentifier(buffer)) {
							lexemes->Add(gcnew Lexeme{ "Identifier", buffer });
						}
						else {
							Console::WriteLine("Invalid identifier '" + buffer + "' at position " + i);
							return nullptr;
						}
						buffer = "";
						operatorFlag = false;
					}
				}
				else if (c == "+" || c == "-" || c == "*" || c == "/") {
					if (operatorFlag) {
						Console::WriteLine("Double operator error at position " + i);
						return nullptr;
					}
					lexemes->Add(gcnew Lexeme{ "Operator", Convert::ToString(c) });
					operatorFlag = true;
					assignmentFlag = false;
				}
				else if (c == "(") {
					openBrackets++;
					lexemes->Add(gcnew Lexeme{ "Parenthesis", Convert::ToString(c) });
					operatorFlag = false;
					assignmentFlag = false;
				}
				else if (c == ")") {
					if (openBrackets == 0) {
						Console::WriteLine("Unexpected closing bracket at position " + i);
						return nullptr;
					}
					openBrackets--;
					lexemes->Add(gcnew Lexeme{ "Parenthesis", Convert::ToString(c) });
					operatorFlag = false;
					assignmentFlag = false;
				}
				else if (c == ":") {
					buffer += c;
				}
				else if (c == "=") {
					if (!String::IsNullOrEmpty(buffer) && buffer[buffer->Length - 1] == ':') {
						buffer += c;
						if (assignmentFlag) {
							Console::WriteLine("Double assignment error at position " + i);
							return nullptr;
						}
						lexemes->Add(gcnew Lexeme{ "Assignment operator", buffer });
						buffer = "";
						assignmentFlag = true;
						operatorFlag = false;
					}
					else {
						Console::WriteLine("Unexpected character '=' at position " + i);
						return nullptr;
					}
				}
				else if (c == ";") {
					lexemes->Add(gcnew Lexeme{ "Statement separator", ";" });
					operatorFlag = false;
					assignmentFlag = false;
				}
				else if (Char::IsWhiteSpace(c[0])) {
					continue;
				}
				else {
					Console::WriteLine("Unexpected character '" + c + "' at position " + i);
					return nullptr;
				}
			}

			if (openBrackets != 0) {
				Console::WriteLine("Unbalanced brackets in the input");
				return nullptr;
			}

			return lexemes;
		}



		// Определение типов для удобства
		typedef System::String^ Symbol;
		typedef System::Collections::Generic::List<Symbol>^ Production;
		typedef System::Collections::Generic::Dictionary<Symbol, System::Collections::Generic::List<Production>^>^ Grammar;
		typedef System::Collections::Generic::Dictionary<Symbol, System::Collections::Generic::Dictionary<Symbol, char>^>^ PrecedenceMatrix;
		int y = 0, yy = 0, yyy = 0;

		void shiftReduce(System::Collections::Generic::List<Symbol>^ input, Grammar grammar, PrecedenceMatrix precedenceMatrix, System::Windows::Forms::TreeView^ treeView) {
			System::Collections::Generic::List<Symbol>^ stack = gcnew System::Collections::Generic::List<Symbol>();
			System::Collections::Generic::List<System::Windows::Forms::TreeNode^>^ nodeStack = gcnew System::Collections::Generic::List<System::Windows::Forms::TreeNode^>();
			int Opcounter = 0;
			// Добавляем символ начала строки в стек
			stack->Add("#");

			// Добавляем символ конца строки в конец входной цепочки
			input->Add("#");

			int i = 0; // Индекс текущего символа во входной цепочке

			// Создаем корневой узел дерева
			System::Windows::Forms::TreeNode^ rootNode = gcnew System::Windows::Forms::TreeNode("E");
			treeView->Nodes->Add(rootNode);
			nodeStack->Add(rootNode);

			// Буфер для хранения текущего римского числа
			System::String^ romanNumeralBuffer = "";

			while (true) {
				// Ищем самый верхний терминальный символ в стеке
				Symbol topTerminal;
				for (int j = stack->Count - 1; j >= 0; --j) {
					if (!grammar->ContainsKey(stack[j])) { // Если символ является терминальным
						topTerminal = stack[j];
						break;
					}
				}

				// Получаем текущий входной символ

				Symbol currentInput = input[i];

				Dictionary<Symbol, char>^ innerDict;

				// Если мы достигли конца строки в стеке и во входной цепочке, то разбор успешно завершен
				if (topTerminal == "#" && currentInput == "#") {
					break;
				}

				if (isIdentifier(currentInput) && !isRomanNumeral(currentInput) && currentInput != "a") {
					// Если текущий ввод - это идентификатор, добавляем его в стек и создаем новый узел в дереве разбора
					stack->Add(currentInput);

					System::Windows::Forms::TreeNode^ newENode = gcnew System::Windows::Forms::TreeNode("E");

					System::Windows::Forms::TreeNode^ newNode = gcnew System::Windows::Forms::TreeNode(currentInput);

					newENode->Nodes->Add(newNode);

					// Добавляем узел "E" к текущему корневому узлу
					nodeStack[nodeStack->Count - 1]->Nodes->Add(newENode);

					// Обновляем корневой узел
					nodeStack->Add(newENode);

					++i;
				}
				// Получаем отношение предшествования между верхним терминальным символом в стеке и текущим входным символом
				
				else if (precedenceMatrix->TryGetValue(topTerminal, innerDict))
				{
					char relation;
					if (innerDict->TryGetValue(currentInput, relation))
					{
						if (relation == '<' || relation == '=') {
							// Если отношение предшествования равно '<' или '=', выполняем сдвиг
							stack->Add(currentInput);
							++i;

							if (currentInput != "a" && currentInput != ":=")
							{
								// Создаем новый узел дерева для "E"

								if (isRomanNumeral(currentInput)) {

									romanNumeralBuffer += currentInput;

									continue;
								}
								else
								{
									if (!String::IsNullOrEmpty(romanNumeralBuffer))
									{
										System::Windows::Forms::TreeNode^ newENode = gcnew System::Windows::Forms::TreeNode("E");

										System::Windows::Forms::TreeNode^ newNode = gcnew System::Windows::Forms::TreeNode(romanNumeralBuffer);

										newENode->Nodes->Add(newNode);

										// Добавляем узел "E" к текущему корневому узлу
										nodeStack[nodeStack->Count - 1]->Nodes->Add(newENode);

										// Обновляем корневой узел
										nodeStack->Add(newENode);

										romanNumeralBuffer = ""; // Очищаем буфер
									}

									if (Opcounter == 0)
									{
										// Добавляем узел к дереву
										System::Windows::Forms::TreeNode^ newENode = gcnew System::Windows::Forms::TreeNode("E");

										System::Windows::Forms::TreeNode^ newNode = gcnew System::Windows::Forms::TreeNode(currentInput);

										newENode->Nodes->Add(newNode);

										// Добавляем узел "E" к текущему корневому узлу
										nodeStack[nodeStack->Count - 1]->Nodes->Add(newENode);

										// Обновляем корневой узел
										nodeStack->Add(newENode);

										Opcounter++;
									}
									else
									{
										// Добавляем узел к дереву
										//System::Windows::Forms::TreeNode^ newENode = gcnew System::Windows::Forms::TreeNode("E");

										System::Windows::Forms::TreeNode^ newNode = gcnew System::Windows::Forms::TreeNode(currentInput);

										// Добавляем узел "E" к текущему корневому узлу
										nodeStack[nodeStack->Count - 2]->Nodes->Add(newNode);

										// Обновляем корневой узел
										nodeStack->Add(newNode);

										//Opcounter--;

									}
								}
							}
							else
							{
								if (!String::IsNullOrEmpty(romanNumeralBuffer))
								{
									System::Windows::Forms::TreeNode^ newENode = gcnew System::Windows::Forms::TreeNode("E");

									System::Windows::Forms::TreeNode^ newNode = gcnew System::Windows::Forms::TreeNode(romanNumeralBuffer);

									newENode->Nodes->Add(newNode);

									// Добавляем узел "E" к текущему корневому узлу
									nodeStack[nodeStack->Count - 1]->Nodes->Add(newENode);

									// Обновляем корневой узел
									nodeStack->Add(newENode);

									romanNumeralBuffer = ""; // Очищаем буфер
								}

								System::Windows::Forms::TreeNode^ newNode = gcnew System::Windows::Forms::TreeNode(currentInput);
								// Добавляем новый узел в стек узлов
								nodeStack->Add(newNode);

								// Добавляем новый узел к текущему корневому узлу
								rootNode->Nodes->Add(newNode);

							}
						}
						else if (relation == '>') {
							hash:
							if (!String::IsNullOrEmpty(romanNumeralBuffer))
							{
								System::Windows::Forms::TreeNode^ newENode = gcnew System::Windows::Forms::TreeNode("E");

								System::Windows::Forms::TreeNode^ newNode = gcnew System::Windows::Forms::TreeNode(romanNumeralBuffer);

								newENode->Nodes->Add(newNode);

								// Добавляем узел "E" к текущему корневому узлу
								nodeStack[nodeStack->Count - 1]->Nodes->Add(newENode);

								// Обновляем корневой узел
								nodeStack->Add(newENode);

								romanNumeralBuffer = ""; // Очищаем буфер
							}

							// Если отношение предшествования равно '>', выполняем свертку
							System::Collections::Generic::List<Symbol>^ symbolsToReduce = gcnew System::Collections::Generic::List<Symbol>();
							System::Collections::Generic::List<System::Windows::Forms::TreeNode^>^ nodesToReduce = gcnew System::Collections::Generic::List<System::Windows::Forms::TreeNode^>();
							while (true) {
								Symbol top = stack[stack->Count - 1];
								symbolsToReduce->Add(top);
								stack->RemoveAt(stack->Count - 1);

								if (stack->Count == 0) {
									break;
								}

								Symbol nextTopTerminal;
								for (int j = stack->Count - 1; j >= 0; --j) {
									if (!grammar->ContainsKey(stack[j])) { // Если символ является терминальным
										nextTopTerminal = stack[j];
										break;
									}
								}

								Dictionary<Symbol, char>^ innerDict2;
								if (precedenceMatrix->TryGetValue(nextTopTerminal, innerDict2))
								{
									char relation2;
									if (innerDict2->TryGetValue(top, relation2)) {
										if (relation2 != '=') {
											break;
										}
									}
								}
							}

							// Проверяем, можно ли свернуть символы
							for each (auto rule in grammar) {
								if (rule.Value->Count != symbolsToReduce->Count) {
									continue;
								}

								bool match = true;
								for (int j = 0; j < rule.Value->Count; ++j) {
									Production production = rule.Value[j];
									Symbol ruleSymbol = production[0]; // Получаем символ из продукции
									Symbol reduceSymbol = symbolsToReduce[symbolsToReduce->Count - 1 - j];
									if (ruleSymbol != reduceSymbol) {
										match = false;
										break;
									}
								}

								if (match) {
									// Если мы нашли соответствующее правило, сворачиваем символы в нетерминал
									stack->Add(rule.Key);

									// Создаем новый узел дерева для нетерминала
									System::Windows::Forms::TreeNode^ newNode = gcnew System::Windows::Forms::TreeNode(rule.Key);

									// Добавляем узлы, которые мы свернули, как дочерние узлы для нового узла
									for each (System::Windows::Forms::TreeNode ^ node in nodesToReduce)
									{
										newNode->Nodes->Add(node);
									}

									// Добавляем новый узел в стек узлов
									nodeStack->Add(newNode);

									rootNode = newNode;

								}
							}
						}
						else {
							// Если отношение предшествования не определено, выводим сообщение об ошибке и прерываем алгоритм
							Console::WriteLine("Error: no such relation in PrecedenceMatrix");
							exit(0);
						}
					}
					else {
						// Если отношение предшествования не определено, выводим сообщение об ошибке и прерываем алгоритм
						Console::WriteLine("Error: no such relation in PrecedenceMatrix");
						exit(0);
					}
				}
				else if (currentInput == "a")
				{
					stack->Add(currentInput);

					System::Windows::Forms::TreeNode^ newNode = gcnew System::Windows::Forms::TreeNode(currentInput);
					// Добавляем новый узел в стек узлов
					nodeStack->Add(newNode);

					// Добавляем новый узел к текущему корневому узлу
					rootNode->Nodes->Add(newNode);

					++i;
				}
				else
				{
					if (currentInput == "#")
						goto hash;
					else
					{
						stack->Add(currentInput);

						System::Windows::Forms::TreeNode^ newNode = gcnew System::Windows::Forms::TreeNode(currentInput);

						// Добавляем узел "E" к текущему корневому узлу
						nodeStack[nodeStack->Count - 1]->Nodes->Add(newNode);

						// Обновляем корневой узел
						nodeStack->Add(newNode);

						++i;
					}
				}
			}
			Console::WriteLine("Разбор успешно завершен.");
		}

		ref class Triad {
		public:
			int number;
			String^ operation;
			String^ operand1;
			String^ operand2;

			Triad(int number, String^ operation, String^ operand1, String^ operand2) {
				this->number = number;
				this->operation = operation;
				this->operand1 = operand1;
				this->operand2 = operand2;
			}

			virtual String^ ToString() override {
				return String::Format("{0}: {1}({2},{3})", number, operation, operand1, operand2);
			}
		};

		bool isConstant(String^ str) {
			return isRomanNumeral(str);
		}


		int romanToInteger(String^ str) {
			int result = 0;
			for each (char c in str) {
				if (c == 'I') result += 1;
				else if (c == 'V') result += 5;
				else if (c == 'X') result += 10;
			}
			if (str->Contains("IV") || str->Contains("IX")) result -= 2;
			if (str->Contains("XL") || str->Contains("XC")) result -= 20;
			return result;
		}

		String^ integerToRoman(int num) {
			String^ result = "";
			Dictionary<String^, int>^ map = gcnew Dictionary<String^, int>();
			map["X"] = 10; map["IX"] = 9; map["V"] = 5; map["IV"] = 4; map["I"] = 1;
			for each (KeyValuePair<String^, int> ^ pair in map) {
				while (num >= pair->Value) {
					result += pair->Key;
					num -= pair->Value;
				}
			}
			return result;
		}

		String^ compute(String^ operation, String^ operand1, String^ operand2) {
			int num1 = romanToInteger(operand1);
			int num2 = romanToInteger(operand2);
			int result;

			if (operation == "+") {
				result = num1 + num2;
			}
			else if (operation == "-") {
				result = num1 - num2;
			}
			else if (operation == "*") {
				result = num1 * num2;
			}
			else if (operation == "/") {
				if (num2 != 0) {
					result = num1 / num2;
				}
				else {
					throw gcnew DivideByZeroException();
				}
			}
			else {
				throw gcnew ArgumentException("Invalid operation");
			}

			return integerToRoman(result);
		}

		int getPriority(String^ operation) {
			if (operation == "*" || operation == "/") {
				return 2;
			}
			else if (operation == "+" || operation == "-") {
				return 1;
			}
			else if (operation == ":=") {
				return 0;
			}
			else {
				return -1; // Для недопустимых операций
			}
		}

		int printTriads(List<Triad^>^ triads, int y, int x) {
			for each (Triad ^ triad in triads) {
				System::Windows::Forms::Label^ label = gcnew System::Windows::Forms::Label();
				label->Text = triad->ToString();
				label->Location = System::Drawing::Point(x, y);
				syntaxPage->Controls->Add(label);
				y += 30;
			}
			return y;
		}

		List<Triad^>^ build_triads(List<Symbol>^ symbols) {
			List<Triad^>^ allTriads = gcnew List<Triad^>();
			List<Symbol>^ lineSymbols = gcnew List<Symbol>();
			int triadCounter = 1;

			for each (Symbol symbol in symbols) {
				if (symbol == "a" && lineSymbols->Count > 0) {
					// Обрабатываем текущую строку
					allTriads->AddRange(processLine(lineSymbols, triadCounter));
					lineSymbols->Clear(); // Очищаем список символов для следующей строки
				}

				lineSymbols->Add(symbol);
			}

			// Обрабатываем последнюю строку
			if (lineSymbols->Count > 0) {
				allTriads->AddRange(processLine(lineSymbols, triadCounter));
			}

			return allTriads;
		}

		List<Triad^>^ processLine(List<Symbol>^ symbols, int& triadCounter) {
			List<Triad^>^ triads = gcnew List<Triad^>();
			System::Collections::Generic::Stack<String^>^ operandStack = gcnew System::Collections::Generic::Stack<String^>();
			System::Collections::Generic::Stack<String^>^ operatorStack = gcnew System::Collections::Generic::Stack<String^>();
			for (int i = 0; i < symbols->Count; ++i) {
				if (symbols[i] == "(") {
					operatorStack->Push(symbols[i]);
				}
				else if (symbols[i] == ")") {
					while (operatorStack->Count > 0 && operatorStack->Peek() != "(") {
						String^ operand2 = operandStack->Pop();
						String^ operation = operatorStack->Pop();
						String^ operand1 = operandStack->Pop();

						Triad^ triad = gcnew Triad(triadCounter++, operation, operand1, operand2);
						triads->Add(triad);

						operandStack->Push("^" + (triadCounter - 1));
					}
					operatorStack->Pop(); // Удаляем открывающую скобку из стека
				}
				else if (symbols[i] == "+" || symbols[i] == "-" || symbols[i] == "*" || symbols[i] == "/") {
					while (operatorStack->Count > 0 && getPriority(operatorStack->Peek()) >= getPriority(symbols[i])) {
						String^ operand2 = operandStack->Pop();
						String^ operation = operatorStack->Pop();
						String^ operand1 = operandStack->Pop();

						Triad^ triad = gcnew Triad(triadCounter++, operation, operand1, operand2);
						triads->Add(triad);

						operandStack->Push("^" + (triadCounter - 1));
					}
					operatorStack->Push(symbols[i]);
				}
				else {
					operandStack->Push(symbols[i]);
				}
			}

			while (operatorStack->Count > 0) {
				String^ operand2 = operandStack->Pop();
				String^ operation = operatorStack->Pop();
				String^ operand1 = operandStack->Pop();

				Triad^ triad = gcnew Triad(triadCounter++, operation, operand1, operand2);
				triads->Add(triad);

				operandStack->Push("^" + (triadCounter - 1));
			}

			// Добавляем операцию :=
			if (operandStack->Count > 0) {
				String^ operand1 = operandStack->Pop();
				Triad^ triad = gcnew Triad(triadCounter++, ":=", "a", operand1);
				triads->Add(triad);
			}

			return triads;
		}




		List<Triad^>^ fold_triads(List<Triad^>^ triads) {
			List<Triad^>^ foldedTriads = gcnew List<Triad^>();
			Dictionary<String^, String^>^ tableT = gcnew Dictionary<String^, String^>();

			for each (Triad ^ triad in triads) {
				String^ operation = triad->operation;
				String^ operand1 = triad->operand1;
				String^ operand2 = triad->operand2;

				if (isConstant(operand1) && isConstant(operand2)) {
					String^ result = compute(operation, operand1, operand2);
					Triad^ newTriad = gcnew Triad(triad->number, ":=", "a", result);
					foldedTriads->Add(newTriad);
					tableT[triad->operand1] = result;
				}
				else if (operation == ":=" && operand2->StartsWith("^")) {
					// Если операция является операцией присваивания и правый операнд является ссылкой на другую триаду,
					// то заменяем ссылку на вычисленное значение этой триады.
					String^ triadReference = operand2->Substring(1);
					if (tableT->ContainsKey(triadReference)) {
						operand2 = tableT[triadReference];
						Triad^ newTriad = gcnew Triad(triad->number, operation, operand1, operand2);
						foldedTriads->Add(newTriad);
					}
				}
				else {
					foldedTriads->Add(triad);
				}
			}

			for (int i = 0; i < foldedTriads->Count; i++) {
				foldedTriads[i]->number = i + 1;
			}

			return foldedTriads;
		}

		List<Triad^>^ remove_redundant_operations(List<Triad^>^ triads) {
			List<Triad^>^ optimizedTriads = gcnew List<Triad^>();
			Dictionary<String^, int>^ dependencyNumbers = gcnew Dictionary<String^, int>();

			for (int i = 0; i < triads->Count; i++) {
				Triad^ triad = triads[i];
				String^ operation = triad->operation;
				String^ operand1 = triad->operand1;
				String^ operand2 = triad->operand2;

				// Вычисляем число зависимости для текущей триады
				int dep1 = dependencyNumbers->ContainsKey(operand1) ? dependencyNumbers[operand1] : 0;
				int dep2 = dependencyNumbers->ContainsKey(operand2) ? dependencyNumbers[operand2] : 0;
				int dep = 1 + Math::Max(dep1, dep2);

				// Проверяем, существует ли идентичная триада с меньшим номером и таким же числом зависимости
				bool isRedundant = false;
				for (int j = 0; j < i; j++) {
					Triad^ otherTriad = triads[j];
					if (otherTriad->operation == operation && otherTriad->operand1 == operand1 && otherTriad->operand2 == operand2) {
						int otherDep = 1 + Math::Max(
							dependencyNumbers->ContainsKey(otherTriad->operand1) ? dependencyNumbers[otherTriad->operand1] : 0,
							dependencyNumbers->ContainsKey(otherTriad->operand2) ? dependencyNumbers[otherTriad->operand2] : 0
						);
						if (otherDep == dep) {
							isRedundant = true;
							break;
						}
					}
				}

				// Если триада не является лишней, добавляем ее в список оптимизированных триад
				if (!isRedundant) {
					optimizedTriads->Add(triad);
					if (operation == ":=") {
						dependencyNumbers[operand1] = dep;
					}
				}
			}

			// Перенумеровываем триады
			for (int i = 0; i < optimizedTriads->Count; i++) {
				optimizedTriads[i]->number = i + 1;
			}

			return optimizedTriads;
		}





		MyForm(void)
		{
			InitializeComponent();
		}

	protected:
		~MyForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private:
		System::Windows::Forms::Button^ button1;
		System::Windows::Forms::Button^ button2;
		System::Windows::Forms::TextBox^ codeTextBox; // TextBox для отображения кода
		System::Windows::Forms::DataGridView^ resultGridView; // DataGridView для отображения результатов
		System::Windows::Forms::DataGridView^ triadsGridView; // DataGridView для отображения триад
		System::Windows::Forms::DataGridView^ foldedTriadsGridView; // DataGridView для отображения свернутых триад
		System::Windows::Forms::DataGridView^ optimizedTriadsGridView; // DataGridView для отображения оптимизированных триад

		System::Windows::Forms::TabControl^ tabControl; // TabControl для переключения между кодом и результатами
		System::Windows::Forms::TabPage^ codePage; // Вкладка для кода
		System::Windows::Forms::TabPage^ resultPage; // Вкладка для результатов
		System::Windows::Forms::TabPage^ treePage; // Вкладка для матрицы предшествования
		System::Windows::Forms::TreeView^ parseTree; // Дерево разбора
		System::Windows::Forms::TabPage^ syntaxPage; // Вкладка для отображения триад
		System::Windows::Forms::Label^ titleLabel;
		System::Windows::Forms::Label^ titleLabel2;
		System::Windows::Forms::Label^ titleLabel3;

	protected:
	private:
		System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
		void InitializeComponent(void)
		{
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->codeTextBox = (gcnew System::Windows::Forms::TextBox());
			this->resultGridView = (gcnew System::Windows::Forms::DataGridView());
			this->tabControl = (gcnew System::Windows::Forms::TabControl());
			this->codePage = (gcnew System::Windows::Forms::TabPage());
			this->resultPage = (gcnew System::Windows::Forms::TabPage());
			this->treePage = (gcnew System::Windows::Forms::TabPage());
			this->syntaxPage = (gcnew System::Windows::Forms::TabPage());
			this->parseTree = (gcnew System::Windows::Forms::TreeView());
			this->titleLabel = (gcnew System::Windows::Forms::Label());
			this->titleLabel2 = (gcnew System::Windows::Forms::Label());
			this->titleLabel3 = (gcnew System::Windows::Forms::Label());

			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->resultGridView))->BeginInit();
			this->tabControl->SuspendLayout();
			this->codePage->SuspendLayout();
			this->resultPage->SuspendLayout();
			this->treePage->SuspendLayout();
			this->syntaxPage->SuspendLayout();
			this->SuspendLayout();
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(570, 12);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(139, 27);
			this->button1->TabIndex = 0;
			this->button1->Text = L"Выбрать файл";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &MyForm::button1_Click);
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(570, 70);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(139, 29);
			this->button2->TabIndex = 0;
			this->button2->Text = L"Загрузить файл";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &MyForm::button2_Click);
			// 
			// codeTextBox
			// 
			this->codeTextBox->Location = System::Drawing::Point(0, 0);
			this->codeTextBox->Multiline = true;
			this->codeTextBox->Name = L"codeTextBox";
			this->codeTextBox->Size = System::Drawing::Size(556, 280);
			this->codeTextBox->TabIndex = 0;
			// 
			// resultGridView
			// 
			this->resultGridView->ColumnHeadersHeight = 29;
			this->resultGridView->Location = System::Drawing::Point(0, 0);
			this->resultGridView->Name = L"resultGridView";
			this->resultGridView->RowHeadersWidth = 51;
			this->resultGridView->Size = System::Drawing::Size(579, 282);
			this->resultGridView->TabIndex = 0;
			this->resultGridView->Columns->Add("Type", "Type");
			this->resultGridView->Columns->Add("Value", "Value");
			// 
			// tabControl
			// 
			this->tabControl->Controls->Add(this->codePage);
			this->tabControl->Controls->Add(this->resultPage);
			this->tabControl->Controls->Add(this->treePage);
			this->tabControl->Controls->Add(this->syntaxPage);
			this->tabControl->Location = System::Drawing::Point(0, 3);
			this->tabControl->Name = L"tabControl";
			this->tabControl->SelectedIndex = 0;
			this->tabControl->Size = System::Drawing::Size(564, 312);
			this->tabControl->TabIndex = 0;
			// 
			// codePage
			// 
			this->codePage->Controls->Add(this->codeTextBox);
			this->codePage->Location = System::Drawing::Point(4, 25);
			this->codePage->Name = L"codePage";
			this->codePage->Size = System::Drawing::Size(556, 283);
			this->codePage->TabIndex = 0;
			this->codePage->Text = L"Программа";
			// 
			// resultPage
			// 
			this->resultPage->Controls->Add(this->resultGridView);
			this->resultPage->Location = System::Drawing::Point(4, 25);
			this->resultPage->Name = L"resultPage";
			this->resultPage->Size = System::Drawing::Size(556, 283);
			this->resultPage->TabIndex = 1;
			this->resultPage->Text = L"Результат";
			// 
			// treePage
			// 
			this->treePage->Controls->Add(this->parseTree);
			this->treePage->Location = System::Drawing::Point(4, 25);
			this->treePage->Name = L"TreePage";
			this->treePage->Size = System::Drawing::Size(556, 283);
			this->treePage->TabIndex = 2;
			this->treePage->Text = L"Синтаксис";

			parseTree->Location = System::Drawing::Point(10, 10);
			parseTree->Size = System::Drawing::Size(536, 263);

			// 
			// syntaxPage
			// 

			this->syntaxPage->Controls->Add(titleLabel);
			this->syntaxPage->Controls->Add(titleLabel2);
			this->syntaxPage->Controls->Add(titleLabel3);
			this->syntaxPage->Location = System::Drawing::Point(4, 25);
			this->syntaxPage->Name = L"syntaxPage";
			this->syntaxPage->Size = System::Drawing::Size(556, 283);
			this->syntaxPage->TabIndex = 3;
			this->syntaxPage->Text = L"Триады";

			this->titleLabel->AutoSize = true;
			this->titleLabel->Text = "Общий список:";
			this->titleLabel->Location = System::Drawing::Point(0, y);
			y += 30; // Увеличиваем y, чтобы следующая метка была ниже заголовка


			int x = syntaxPage->Width / 3;
			this->titleLabel2->AutoSize = true;
			this->titleLabel2->Text = "После свертки:";
			this->titleLabel2->Location = System::Drawing::Point(x, yy);
			yy += 30;

			x = syntaxPage->Width * 3 / 4;
			this->titleLabel3->AutoSize = true;
			this->titleLabel3->Text = "Без лишних операций:";
			this->titleLabel3->Location = System::Drawing::Point(x, yyy);
			yyy += 30;

			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(4, 6);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(768, 391);
			this->Controls->Add(this->tabControl);
			this->Controls->Add(this->button2);
			this->Controls->Add(this->button1);
			this->Name = L"MyForm";
			this->Text = L"MyForm";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->resultGridView))->EndInit();
			this->tabControl->ResumeLayout(false);
			this->codePage->ResumeLayout(false);
			this->codePage->PerformLayout();
			this->resultPage->ResumeLayout(false);
			this->treePage->ResumeLayout(false);
			this->syntaxPage->ResumeLayout(false);
			this->ResumeLayout(false);

		}
#pragma endregion
	private:
		System::Void button1_Click(System::Object^ sender, System::EventArgs^ e)
		{
			OpenFileDialog^ openFileDialog = gcnew OpenFileDialog();
			openFileDialog->Filter = "txt files (*.txt)|*.txt|All files (*.*)|*.*";
			openFileDialog->FileOk += gcnew System::ComponentModel::CancelEventHandler(this, &MyForm::OpenFileDialog_FileOk);
			openFileDialog->ShowDialog();
		}
	private:
		System::Void OpenFileDialog_FileOk(System::Object^ sender, System::ComponentModel::CancelEventArgs^ e) {
			String^ filePath = ((OpenFileDialog^)sender)->FileName;
			StreamReader^ sr = gcnew StreamReader(filePath);
			codeTextBox->Text = sr->ReadToEnd();
			sr->Close();
		}
	private:
		System::Void button2_Click(System::Object^ sender, System::EventArgs^ e) {
			if (!String::IsNullOrEmpty(codeTextBox->Text))
			{
				List<Lexeme^>^ lexemes = tokenize(codeTextBox->Text);
				List<Symbol>^ symbols = gcnew List<Symbol>();
				List<Symbol>^ symbolsdef = gcnew List<Symbol>();
				if (lexemes != nullptr) {
					for each (Lexeme ^ lexeme in lexemes) {
						resultGridView->Rows->Add(lexeme->type, lexeme->value);
					}

					for each (Lexeme ^ lexeme in lexemes) {
						if (lexeme->type == "Roman numeral") {
							// Если лексема - это римское число, разбиваем его на отдельные символы
							for each (char numeral in lexeme->value) {
								symbols->Add(gcnew String(numeral, 1));

							}
						}
						else {
							symbols->Add(lexeme->value);
						}
					}

					for each (Lexeme ^ lexeme in lexemes) {
						symbolsdef->Add(lexeme->value);
					}
				}



				tabControl->SelectedTab = resultPage; // Переключаемся на вкладку с результатами

				if (symbols->Count > 0) {
					Grammar grammar = gcnew Dictionary<Symbol, List<Production>^>();

					// Заполняем грамматику
					grammar["S"] = gcnew List<Production>();
					List<Symbol>^ productionS = gcnew List<Symbol>();
					productionS->Add("a");
					productionS->Add(":=");
					productionS->Add("R"); // Используем "R" для обозначения любого римского числа
					grammar["S"]->Add(productionS);

					grammar["R"] = gcnew List<Production>();
					List<Symbol>^ productionR1 = gcnew List<Symbol>();
					productionR1->Add("I"); // Для римского числа 1
					grammar["R"]->Add(productionR1);
					List<Symbol>^ productionR2 = gcnew List<Symbol>();
					productionR2->Add("V"); // Для римского числа 5
					grammar["R"]->Add(productionR2);
					List<Symbol>^ productionR3 = gcnew List<Symbol>();
					productionR3->Add("X"); // Для римского числа 10
					grammar["R"]->Add(productionR3);

					grammar["D"] = gcnew List<Production>();
					List<Symbol>^ production1 = gcnew List<Symbol>();
					production1->Add("X");
					production1->Add("+");
					production1->Add("V");
					grammar["D"]->Add(production1);
					List<Symbol>^ production2 = gcnew List<Symbol>();
					production2->Add("V");
					grammar["D"]->Add(production2);

					grammar["L"] = gcnew List<Production>();
					List<Symbol>^ production3 = gcnew List<Symbol>();
					production3->Add("V");
					production3->Add("*");
					production3->Add("I");
					grammar["L"]->Add(production3);
					List<Symbol>^ production4 = gcnew List<Symbol>();
					production4->Add("V");
					production4->Add("/");
					production4->Add("I");
					grammar["L"]->Add(production4);
					List<Symbol>^ production5 = gcnew List<Symbol>();
					production5->Add("I");
					grammar["L"]->Add(production5);

					grammar["P"] = gcnew List<Production>();
					List<Symbol>^ production6 = gcnew List<Symbol>();
					production6->Add("(X)");
					grammar["P"]->Add(production6);
					List<Symbol>^ production7 = gcnew List<Symbol>();
					production7->Add("(-X)");
					grammar["P"]->Add(production7);
					List<Symbol>^ production8 = gcnew List<Symbol>();
					production8->Add("a");
					grammar["P"]->Add(production8);

					PrecedenceMatrix precedenceMatrix = gcnew Dictionary<Symbol, Dictionary<Symbol, char>^>();

					// Заполняем матрицу предшествования
					precedenceMatrix["#"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix["#"]->Add("a", '<');

					precedenceMatrix["a"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix["a"]->Add(":=", '<');
					precedenceMatrix["a"]->Add("#", '>');

					precedenceMatrix[":="] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix[":="]->Add("X", '<');
					precedenceMatrix[":="]->Add("V", '<');
					precedenceMatrix[":="]->Add("I", '<');
					precedenceMatrix[":="]->Add("a", '<');
					precedenceMatrix[":="]->Add("(", '<');
					precedenceMatrix[":="]->Add("+", '=');
					precedenceMatrix[":="]->Add("-", '=');
					precedenceMatrix[":="]->Add("*", '=');
					precedenceMatrix[":="]->Add("/", '=');
					precedenceMatrix[":="]->Add("#", '>');

					precedenceMatrix["X"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix["X"]->Add("X", '<');
					precedenceMatrix["X"]->Add("V", '<');
					precedenceMatrix["X"]->Add("I", '<');
					precedenceMatrix["X"]->Add("a", '<');
					precedenceMatrix["X"]->Add(";", '<');
					precedenceMatrix["X"]->Add("+", '<');
					precedenceMatrix["X"]->Add("/", '<');
					precedenceMatrix["X"]->Add("*", '<');
					precedenceMatrix["X"]->Add(":=", '>');
					precedenceMatrix["X"]->Add(")", '<');
					precedenceMatrix["X"]->Add("(", '>');
					precedenceMatrix["X"]->Add("-", '<');
					precedenceMatrix["X"]->Add("#", '>');

					precedenceMatrix["V"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix["V"]->Add(":=", '>');
					precedenceMatrix["V"]->Add("I", '<');
					precedenceMatrix["V"]->Add("+", '<');
					precedenceMatrix["V"]->Add("-", '<');
					precedenceMatrix["V"]->Add("*", '<');
					precedenceMatrix["V"]->Add("/", '<');
					precedenceMatrix["V"]->Add("a", '<');
					precedenceMatrix["V"]->Add(";", '<');
					precedenceMatrix["V"]->Add("#", '>');
					precedenceMatrix["V"]->Add("(", '>');
					precedenceMatrix["V"]->Add(")", '<');

					precedenceMatrix["I"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix["I"]->Add("V", '<');
					precedenceMatrix["I"]->Add("X", '<');
					precedenceMatrix["I"]->Add(":=", '>');
					precedenceMatrix["I"]->Add("I", '<');
					precedenceMatrix["I"]->Add("+", '<');
					precedenceMatrix["I"]->Add("-", '<');
					precedenceMatrix["I"]->Add("*", '<');
					precedenceMatrix["I"]->Add("/", '<');
					precedenceMatrix["I"]->Add("a", '<');
					precedenceMatrix["I"]->Add(";", '<');
					precedenceMatrix["I"]->Add("(", '>');
					precedenceMatrix["I"]->Add(")", '<');
					precedenceMatrix["I"]->Add("#", '>');

					precedenceMatrix["+"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix["+"]->Add("V", '<');
					precedenceMatrix["+"]->Add("X", '<');
					precedenceMatrix["+"]->Add("I", '<');
					precedenceMatrix["+"]->Add("(", '<');
					precedenceMatrix["+"]->Add(")", '>');
					precedenceMatrix["+"]->Add(":=", '>');
					precedenceMatrix["+"]->Add("+", '=');
					precedenceMatrix["+"]->Add("-", '=');
					precedenceMatrix["+"]->Add("*", '=');
					precedenceMatrix["+"]->Add("/", '=');
					precedenceMatrix["+"]->Add("#", '>');

					precedenceMatrix["-"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix["-"]->Add("(", '<');
					precedenceMatrix["-"]->Add(")", '>');
					precedenceMatrix["-"]->Add(":=", '>');
					precedenceMatrix["-"]->Add("V", '<');
					precedenceMatrix["-"]->Add("X", '<');
					precedenceMatrix["-"]->Add("I", '<');
					precedenceMatrix["-"]->Add("+", '=');
					precedenceMatrix["-"]->Add("-", '=');
					precedenceMatrix["-"]->Add("*", '=');
					precedenceMatrix["-"]->Add("/", '=');
					precedenceMatrix["-"]->Add("#", '>');

					precedenceMatrix["*"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix["*"]->Add("(", '<');
					precedenceMatrix["*"]->Add(")", '>');
					precedenceMatrix["*"]->Add("I", '<');
					precedenceMatrix["*"]->Add(":=", '>');
					precedenceMatrix["*"]->Add("V", '<');
					precedenceMatrix["*"]->Add("X", '<');
					precedenceMatrix["*"]->Add("+", '=');
					precedenceMatrix["*"]->Add("-", '=');
					precedenceMatrix["*"]->Add("*", '=');
					precedenceMatrix["*"]->Add("/", '=');
					precedenceMatrix["*"]->Add("#", '>');

					precedenceMatrix["/"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix["/"]->Add("(", '<');
					precedenceMatrix["/"]->Add(")", '>');
					precedenceMatrix["/"]->Add("I", '<');
					precedenceMatrix["/"]->Add(":=", '>');
					precedenceMatrix["/"]->Add("V", '<');
					precedenceMatrix["/"]->Add("X", '<');
					precedenceMatrix["/"]->Add("+", '=');
					precedenceMatrix["/"]->Add("-", '=');
					precedenceMatrix["/"]->Add("*", '=');
					precedenceMatrix["/"]->Add("/", '=');
					precedenceMatrix["/"]->Add("#", '>');

					precedenceMatrix["("] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix["("]->Add("X", '<');
					precedenceMatrix["("]->Add(":=", '>');
					precedenceMatrix["("]->Add(")", '<');
					precedenceMatrix["("]->Add("-", '>');
					precedenceMatrix["("]->Add("V", '<');
					precedenceMatrix["("]->Add("I", '<');
					precedenceMatrix["("]->Add("#", '>');

					precedenceMatrix[")"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix[")"]->Add("X", '>');
					precedenceMatrix[")"]->Add(":=", '>');
					precedenceMatrix[")"]->Add("a", '<');
					precedenceMatrix[")"]->Add("-", '<');
					precedenceMatrix[")"]->Add("+", '<');
					precedenceMatrix[")"]->Add("*", '<');
					precedenceMatrix[")"]->Add("/", '<');
					precedenceMatrix[")"]->Add("V", '>');
					precedenceMatrix[")"]->Add("I", '>');
					precedenceMatrix[")"]->Add("#", '>');

					precedenceMatrix[";"] = gcnew Dictionary<Symbol, char>();
					precedenceMatrix[";"]->Add("#", '>');
					precedenceMatrix[";"]->Add("a", '<');

					
					List<Symbol>^ filteredSymbols = gcnew List<Symbol>();

					//фильтровка для дерева разбора
					for (int i = symbols->Count - 1; i >= 0; --i) {
						if (symbols[i] == "(" || symbols[i] == ")") {
							symbols->RemoveAt(i);
						}
					}
					
					//Выполнение синтаксического анализа и построение дерева разбора
					shiftReduce(symbols, grammar, precedenceMatrix, parseTree);

					List<Triad^>^ triads = build_triads(symbolsdef);

					// Выводим общий список триад

					y = printTriads(triads, y, 0);

					int x = syntaxPage->Width / 3;

					// Свертка триад
					List<Triad^>^ foldedTriads = fold_triads(triads);

					// Выводим список триад после свертки справа от общего списка

					printTriads(foldedTriads, yy, x);

					// Исключение лишних операций
					List<Triad^>^ optimizedTriads = remove_redundant_operations(foldedTriads);

					// Выводим список триад после исключения лишних операций еще правее

					x = syntaxPage->Width * 3 / 4;

					printTriads(optimizedTriads, yyy, x);



				}

			}
			else
			{
				MessageBox::Show("Пожалуйста, сначала выберите файл.");
			}
		}

	};

}