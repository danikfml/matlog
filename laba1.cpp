#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <locale>
#include <windows.h>
#include <memory>

// Определение списка аксиом
const std::vector<std::string> baseAxioms = {
        "p->(q->p)",  // А1
        "(s->(p->q))->((s->p)->(s->q))",  // А2
        "((p->f)->f)->p"  // А3
};

// Имена аксиом для отображения
const std::vector<std::string> axiomLabels = {
        "K",
        "S",
        "E¬"
};

// Класс, представляющий узел выражения
class Node {
public:
    char symbol;  // значение узла
    std::unique_ptr<Node> leftChild;  // левый дочерний узел
    std::unique_ptr<Node> rightChild;  // правый дочерний узел

    // Конструктор узла
    explicit Node(char val) : symbol(val), leftChild(nullptr), rightChild(nullptr) {}

    // Статический метод для создания узла из формулы
    static std::unique_ptr<Node> constructFromFormula(const std::string& formula);
};

// Класс, представляющий дерево выражений
class ExpressionTree {
public:
    std::string expr;  // исходная формула
    std::unique_ptr<Node> root;  // корень дерева

    // Конструктор дерева
    explicit ExpressionTree(std::string expression) : expr(std::move(expression)), root(nullptr) {
        root = Node::constructFromFormula(expr);
    }
};

// Структура для результата сравнения узлов
struct NodeCompareResult {
    int compareCode;  // результат сравнения
    const Node* axiomNode;  // узел аксиомы
    const Node* formulaNode;  // узел формулы

    NodeCompareResult() : compareCode(0), axiomNode(nullptr), formulaNode(nullptr) {}
    NodeCompareResult(int res, const Node* axiom, const Node* formula)
            : compareCode(res), axiomNode(axiom), formulaNode(formula) {}
};

// Структура для бета-вывода
struct BetaResult {
    bool derived;  // флаг успешного вывода
    int axiomIndex;  // индекс аксиомы
    char variable;  // переменная
    std::string subExpr;  // подформула

    BetaResult() : derived(false), axiomIndex(0), variable(' '), subExpr("") {}
    BetaResult(bool success, int index, char var, std::string subf)
            : derived(success), axiomIndex(index), variable(var), subExpr(std::move(subf)) {}
};

// Метод для создания узла выражения из формулы
std::unique_ptr<Node> Node::constructFromFormula(const std::string& formula) {
    if (formula.size() == 1 && islower(formula[0])) {
        return std::make_unique<Node>(formula[0]);
    }

    std::stack<std::unique_ptr<Node>> nodeStack;
    for (int i = formula.size() - 1; i >= 0; --i) {
        if (formula[i] == ')') {
            size_t index = i;
            int balance = -1;
            // Найти соответствующую открывающую скобку
            while (balance != 0) {
                --index;
                if (formula[index] == ')') --balance;
                else if (formula[index] == '(') ++balance;
            }
            nodeStack.push(constructFromFormula(formula.substr(index + 1, i - index - 1)));
            i = index;
        } else if (islower(formula[i])) {
            nodeStack.push(std::make_unique<Node>(formula[i]));
        } else if (formula[i] == '-' && formula[i + 1] == '>') {
            auto currentNode = std::make_unique<Node>('>');
            currentNode->leftChild = constructFromFormula(formula.substr(0, i));
            currentNode->rightChild = std::move(nodeStack.top());
            nodeStack.pop();
            return currentNode;
        }
    }
    return std::move(nodeStack.top());
}

// Функция для преобразования узла в строку
std::string nodeToString(const Node& node) {
    if (!node.leftChild && !node.rightChild) return std::string(1, node.symbol);
    std::string result;
    if (node.leftChild) result += nodeToString(*node.leftChild);
    result += "->";
    if (node.rightChild) result += nodeToString(*node.rightChild);
    return "(" + result + ")";
}

// Функция для корректировки формулы
std::string refineFormula(const std::string& formula) {
    std::stack<char> bracketStack;
    std::vector<bool> charUsage(formula.size(), true);
    bool leftExists = false, rightNeeded = false, correctionRequired = false, operationNeeded = false;

    for (size_t i = 0; i < formula.size(); ++i) {
        if (formula[i] == '(') {
            if (operationNeeded) throw std::string("Необходима операция между переменными");
            leftExists = false;
            bracketStack.push('(');
        } else if (formula[i] == ')') {
            if (rightNeeded) throw std::string("Необходима операция между переменными");
            if (!bracketStack.empty() && bracketStack.top() == '(') {
                bracketStack.pop();
            } else {
                throw std::string("Некорректная последовательность скобок");
            }
        } else if (formula[i] == '-' && formula[i + 1] == '>') {
            if (!leftExists) throw std::string("Нет переменной перед операцией");
            leftExists = false;
            rightNeeded = true;
            operationNeeded = false;
            i++;
        } else if (islower(formula[i])) {
            if (leftExists) throw std::string("Необходима операция между переменными");
            if (rightNeeded) rightNeeded = false;
            leftExists = true;
            operationNeeded = true;
        } else if (formula[i] == ' ') {
            charUsage[i] = false;
            correctionRequired = true;
        } else {
            throw std::string("Недопустимый символ");
        }
    }
    if (!bracketStack.empty()) throw std::string("Некорректная последовательность скобок");
    if (rightNeeded) throw std::string("Нет правого параметра для операции");

    std::string correctedFormula = formula;
    if (correctionRequired) {
        size_t j = 0;
        for (size_t i = 0; i < formula.size(); ++i) {
            if (charUsage[i]) {
                correctedFormula[j++] = formula[i];
            }
        }
        correctedFormula.resize(j);
    }
    if (correctedFormula.empty()) throw std::string("Полученная формула пуста");
    return correctedFormula;
}

// Функция для разбора формулы
std::string interpretFormula(const std::string& formula) {
    std::string newFormula;
    try {
        newFormula = refineFormula(formula);
    } catch (const std::string& errorMsg) {
        std::cout << "Ошибка формулы: " << errorMsg << '\n';
    }
    return newFormula;
}

// Функция для построения дерева из формулы
std::unique_ptr<ExpressionTree> buildTreeFromFormula(const std::string& formula) {
    std::string correctedFormula = interpretFormula(formula);
    return std::make_unique<ExpressionTree>(correctedFormula);
}

// Функция для сравнения узлов
bool compareNodes(const Node& node1, const Node& node2) {
    if (node1.symbol != node2.symbol) return false;
    if (node1.leftChild && node2.leftChild) {
        if (!compareNodes(*node1.leftChild, *node2.leftChild)) return false;
    } else if (node1.leftChild || node2.leftChild) {
        return false;
    }
    if (node1.rightChild && node2.rightChild) {
        if (!compareNodes(*node1.rightChild, *node2.rightChild)) return false;
    } else if (node1.rightChild || node2.rightChild) {
        return false;
    }
    return true;
}

// Функция для сравнения деревьев
NodeCompareResult compareTrees(const Node& axiom, const Node& formula) {
    if (axiom.symbol == formula.symbol) {
        if (!axiom.leftChild && !formula.leftChild && !axiom.rightChild && !formula.rightChild) {
            return {1, nullptr, nullptr};  // Листовой узел
        } else {
            NodeCompareResult leftResult(1, nullptr, nullptr);
            if (axiom.leftChild && formula.leftChild) {
                leftResult = compareTrees(*axiom.leftChild, *formula.leftChild);
            }
            NodeCompareResult rightResult(1, nullptr, nullptr);
            if (axiom.rightChild && formula.rightChild) {
                rightResult = compareTrees(*axiom.rightChild, *formula.rightChild);
            }
            if (leftResult.compareCode == 0 || rightResult.compareCode == 0) return {};
            if (leftResult.compareCode == 1 && rightResult.compareCode == 1) return {1, nullptr, nullptr};
            if (leftResult.compareCode == 1) return rightResult;
            if (rightResult.compareCode == 1) return leftResult;
            if (leftResult.axiomNode->symbol == rightResult.axiomNode->symbol) {
                return leftResult;
            } else {
                return {};
            }
        }
    } else {
        if (axiom.symbol == 'f' || axiom.symbol == '>') {
            return {};
        } else {
            return {2, &axiom, &formula};  // Переменная найдена
        }
    }
}

// Функция для клонирования узла
void cloneNode(Node& dest, const Node& src) {
    dest.symbol = src.symbol;
    if (src.leftChild) {
        dest.leftChild = std::make_unique<Node>(src.leftChild->symbol);
        cloneNode(*dest.leftChild, *src.leftChild);
    }
    if (src.rightChild) {
        dest.rightChild = std::make_unique<Node>(src.rightChild->symbol);
        cloneNode(*dest.rightChild, *src.rightChild);
    }
}

// Функция для замены значения узла
void substituteNodeValue(char target, const Node& replacement, Node& root) {
    if (root.symbol == target) {
        cloneNode(root, replacement);
    } else {
        if (root.leftChild) substituteNodeValue(target, replacement, *root.leftChild);
        if (root.rightChild) substituteNodeValue(target, replacement, *root.rightChild);
    }
}

// Функция для проверки бета-вывода
BetaResult checkBetaDerivation(const std::vector<std::unique_ptr<ExpressionTree>>& axioms, const ExpressionTree& formula) {
    for (int i = 0; i < axioms.size(); ++i) {
        NodeCompareResult result = compareTrees(*axioms[i]->root, *formula.root);
        if (result.compareCode == 2) {
            auto modifiedTree = std::make_unique<ExpressionTree>(axioms[i]->expr);
            substituteNodeValue(result.axiomNode->symbol, *result.formulaNode, *modifiedTree->root);
            if (compareNodes(*modifiedTree->root, *formula.root)) {
                std::string subExpr = nodeToString(*result.formulaNode);
                if (subExpr.size() >= 3) {
                    subExpr = subExpr.substr(1, subExpr.size() - 2);
                }
                return {true, i, result.axiomNode->symbol, subExpr};
            }
        }
    }
    return {};
}

// Функция для проверки Modus Ponens
std::pair<int, int> checkModusPonens(const std::vector<std::unique_ptr<ExpressionTree>>& axioms, const ExpressionTree& formula) {
    std::vector<size_t> candidates;
    for (size_t i = 0; i < axioms.size(); ++i) {
        if (compareNodes(*axioms[i]->root->rightChild, *formula.root)) {
            candidates.push_back(i);
        }
    }
    std::pair<int, int> result = {-1, -1};
    for (size_t candidate : candidates) {
        result.first = candidate;
        for (int i = 0; i < axioms.size(); ++i) {
            if (compareNodes(*axioms[candidate]->root->leftChild, *axioms[i]->root)) {
                result.second = i;
                break;
            }
        }
        if (result.second != -1) break;
    }
    return result;
}

// Главная функция
int main() {
    setlocale(LC_ALL, "Russian");

    // Создание деревьев для аксиом
    std::vector<std::unique_ptr<ExpressionTree>> axiomsTrees;
    for (const auto& axiom : baseAxioms) {
        axiomsTrees.push_back(buildTreeFromFormula(axiom));
    }

    std::string userInput;
    while (true) {
        std::cout << "Введите формулу или 'exit' для завершения программы: ";
        std::getline(std::cin, userInput);
        if (userInput == "exit") break;
        if (userInput.empty()) continue;

        // Разбор и корректировка введенной формулы
        std::string correctInput = interpretFormula(userInput);
        if (!correctInput.empty()) {
            auto inputTree = buildTreeFromFormula(correctInput);
            bool isEquivalent = false;
            int equivalentIndex = 0;
            for (int i = 0; i < axiomsTrees.size(); ++i) {
                if (compareNodes(*axiomsTrees[i]->root, *inputTree->root)) {
                    isEquivalent = true;
                    equivalentIndex = i;
                    break;
                }
            }
            if (isEquivalent) {
                std::cout << "Формула " << inputTree->expr << " эквивалентна ";
                if (equivalentIndex < baseAxioms.size()) {
                    std::cout << "аксиоме " << axiomLabels[equivalentIndex];
                } else {
                    std::cout << "формуле " << axiomsTrees[equivalentIndex]->expr;
                }
                std::cout << ".\n";
            } else {
                BetaResult betaResult = checkBetaDerivation(axiomsTrees, *inputTree);
                if (betaResult.derived) {
                    std::cout << "Формула " << inputTree->expr << " выводится из ";
                    if (betaResult.axiomIndex < baseAxioms.size()) {
                        std::cout << "аксиоме " << axiomLabels[betaResult.axiomIndex];
                    } else {
                        std::cout << "формулы " << axiomsTrees[betaResult.axiomIndex]->expr;
                    }
                    std::cout << " с заменой переменной \"" << betaResult.variable << "\" на \"" << betaResult.subExpr << "\".\n";
                    axiomsTrees.push_back(std::move(inputTree));
                    continue;
                } else {
                    std::pair<int, int> mpResult = checkModusPonens(axiomsTrees, *inputTree);
                    if (mpResult.first != -1 && mpResult.second != -1) {
                        std::cout << "Формула " << inputTree->expr << " выводима из ";
                        if (mpResult.first < baseAxioms.size()) {
                            std::cout << "аксиоме " << axiomLabels[mpResult.first];
                        } else {
                            std::cout << "формуле " << axiomsTrees[mpResult.first]->expr;
                        }
                        std::cout << " и ";
                        if (mpResult.second < baseAxioms.size()) {
                            std::cout << "аксиоме " << axiomLabels[mpResult.second];
                        } else {
                            std::cout << "формуле " << axiomsTrees[mpResult.second]->expr;
                        }
                        std::cout << " по правилу modus ponens.\n";
                        axiomsTrees.push_back(std::move(inputTree));
                        continue;
                    }
                }
                std::cout << "Формула не выводима.\n";
            }
        }
    }

    return 0;
}
