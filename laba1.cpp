#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <locale>
#include <windows.h>
#include <memory>

// Определение списка аксиом
const std::vector<std::string> axiomsList = {
        "p->(q->p)",  // А1
        "(s->(p->q))->((s->p)->(s->q))",  // А2
        "((p->f)->f)->p"  // А3
};

// Имена аксиом для отображения
const std::vector<std::string> axiomsNames = {
        "K",
        "S",
        "E¬"
};

// Класс, представляющий узел выражения
class ExpressionNode {
public:
    char value;  // значение узла
    std::unique_ptr<ExpressionNode> leftChild;
    std::unique_ptr<ExpressionNode> rightChild;

    // Конструктор узла
    explicit ExpressionNode(char val) : value(val), leftChild(nullptr), rightChild(nullptr) {}

    // Статический метод для создания узла из формулы
    static std::unique_ptr<ExpressionNode> createFromFormula(const std::string& formula);
};

// Класс, представляющий дерево выражений
class ExpressionTree {
public:
    std::string formula;  // исходная формула
    std::unique_ptr<ExpressionNode> rootNode;
    // Конструктор дерева
    explicit ExpressionTree(std::string expr) : formula(std::move(expr)), rootNode(nullptr) {
        rootNode = ExpressionNode::createFromFormula(formula);
    }
};

// Структура для результата сравнения узлов
struct NodeComparisonResult {
    int resultCode;  // результат сравнения
    const ExpressionNode* axiomNode;  // узел аксиомы
    const ExpressionNode* formulaNode;  // узел формулы

    NodeComparisonResult() : resultCode(0), axiomNode(nullptr), formulaNode(nullptr) {}
    NodeComparisonResult(int res, const ExpressionNode* axiom, const ExpressionNode* formula)
            : resultCode(res), axiomNode(axiom), formulaNode(formula) {}
};

// Структура для бета-вывода
struct BetaDerivationResult {
    bool isSuccessful;  // флаг успешного вывода
    int axiomIdx;  // индекс аксиомы
    char variable;  // переменная
    std::string subFormula;  // подформула

    BetaDerivationResult() : isSuccessful(false), axiomIdx(0), variable(' '), subFormula("") {}
    BetaDerivationResult(bool success, int index, char var, std::string subf)
            : isSuccessful(success), axiomIdx(index), variable(var), subFormula(std::move(subf)) {}
};

// Метод для создания узла выражения из формулы
std::unique_ptr<ExpressionNode> ExpressionNode::createFromFormula(const std::string& formula) {
    if (formula.size() == 1 && islower(formula[0])) {
        return std::make_unique<ExpressionNode>(formula[0]);
    }

    std::stack<std::unique_ptr<ExpressionNode>> nodesStack;
    for (int i = formula.size() - 1; i >= 0; --i) {
        if (formula[i] == ')') {
            size_t idx = i;
            int balance = -1;
            // Найти соответствующую открывающую скобку
            while (balance != 0) {
                --idx;
                if (formula[idx] == ')') --balance;
                else if (formula[idx] == '(') ++balance;
            }
            nodesStack.push(createFromFormula(formula.substr(idx + 1, i - idx - 1)));
            i = idx;
        } else if (islower(formula[i])) {
            nodesStack.push(std::make_unique<ExpressionNode>(formula[i]));
        } else if (formula[i] == '-' && formula[i + 1] == '>') {
            auto currentNode = std::make_unique<ExpressionNode>('>');
            currentNode->leftChild = createFromFormula(formula.substr(0, i));
            currentNode->rightChild = std::move(nodesStack.top());
            nodesStack.pop();
            return currentNode;
        }
    }
    return std::move(nodesStack.top());
}

// Функция для преобразования узла в строку
std::string nodeToString(const ExpressionNode& node) {
    if (!node.leftChild && !node.rightChild) return std::string(1, node.value);
    std::string result;
    if (node.leftChild) result += nodeToString(*node.leftChild);
    result += "->";
    if (node.rightChild) result += nodeToString(*node.rightChild);
    return "(" + result + ")";
}

// Функция для корректировки формулы
std::string adjustFormula(const std::string& formula) {
    std::stack<char> brackets;
    std::vector<bool> usageFlags(formula.size(), true);
    bool leftParamExists = false, rightParamRequired = false, correctionRequired = false, operationRequired = false;

    for (size_t i = 0; i < formula.size(); ++i) {
        if (formula[i] == '(') {
            if (operationRequired) throw std::string("Необходима операция между переменными");
            leftParamExists = false;
            brackets.push('(');
        } else if (formula[i] == ')') {
            if (rightParamRequired) throw std::string("Необходима операция между переменными");
            if (!brackets.empty() && brackets.top() == '(') {
                brackets.pop();
            } else {
                throw std::string("Некорректная последовательность скобок");
            }
        } else if (formula[i] == '-' && formula[i + 1] == '>') {
            if (!leftParamExists) throw std::string("Нет переменной перед операцией");
            leftParamExists = false;
            rightParamRequired = true;
            operationRequired = false;
            i++;
        } else if (islower(formula[i])) {
            if (leftParamExists) throw std::string("Необходима операция между переменными");
            if (rightParamRequired) rightParamRequired = false;
            leftParamExists = true;
            operationRequired = true;
        } else if (formula[i] == ' ') {
            usageFlags[i] = false;
            correctionRequired = true;
        } else {
            throw std::string("Недопустимый символ");
        }
    }
    if (!brackets.empty()) throw std::string("Некорректная последовательность скобок");
    if (rightParamRequired) throw std::string("Нет правого параметра для операции");

    std::string correctedFormula = formula;
    if (correctionRequired) {
        size_t j = 0;
        for (size_t i = 0; i < formula.size(); ++i) {
            if (usageFlags[i]) {
                correctedFormula[j++] = formula[i];
            }
        }
        correctedFormula.resize(j);
    }
    if (correctedFormula.empty()) throw std::string("Полученная формула пуста");
    return correctedFormula;
}

// Функция для разбора формулы
std::string parseFormula(const std::string& formula) {
    std::string newFormula;
    try {
        newFormula = adjustFormula(formula);
    } catch (const std::string& errorMsg) {
        std::cout << "Ошибка формулы: " << errorMsg << '\n';
    }
    return newFormula;
}

// Функция для построения дерева из формулы
std::unique_ptr<ExpressionTree> createTreeFromFormula(const std::string& formula) {
    std::string correctedFormula = parseFormula(formula);
    return std::make_unique<ExpressionTree>(correctedFormula);
}

// Функция для сравнения узлов
bool compareExpressionNodes(const ExpressionNode& node1, const ExpressionNode& node2) {
    if (node1.value != node2.value) return false;
    if (node1.leftChild && node2.leftChild) {
        if (!compareExpressionNodes(*node1.leftChild, *node2.leftChild)) return false;
    } else if (node1.leftChild || node2.leftChild) {
        return false;
    }
    if (node1.rightChild && node2.rightChild) {
        if (!compareExpressionNodes(*node1.rightChild, *node2.rightChild)) return false;
    } else if (node1.rightChild || node2.rightChild) {
        return false;
    }
    return true;
}

// Функция для сравнения деревьев
NodeComparisonResult compareExpressionTrees(const ExpressionNode& axiom, const ExpressionNode& formula) {
    if (axiom.value == formula.value) {
        if (!axiom.leftChild && !formula.leftChild && !axiom.rightChild && !formula.rightChild) {
            return {1, nullptr, nullptr};  // Листовой узел
        } else {
            NodeComparisonResult leftResult(1, nullptr, nullptr);
            if (axiom.leftChild && formula.leftChild) {
                leftResult = compareExpressionTrees(*axiom.leftChild, *formula.leftChild);
            }
            NodeComparisonResult rightResult(1, nullptr, nullptr);
            if (axiom.rightChild && formula.rightChild) {
                rightResult = compareExpressionTrees(*axiom.rightChild, *formula.rightChild);
            }
            if (leftResult.resultCode == 0 || rightResult.resultCode == 0) return {};
            if (leftResult.resultCode == 1 && rightResult.resultCode == 1) return {1, nullptr, nullptr};
            if (leftResult.resultCode == 1) return rightResult;
            if (rightResult.resultCode == 1) return leftResult;
            if (leftResult.axiomNode->value == rightResult.axiomNode->value) {
                return leftResult;
            } else {
                return {};
            }
        }
    } else {
        if (axiom.value == 'f' || axiom.value == '>') {
            return {};
        } else {
            return {2, &axiom, &formula};  // Переменная найдена
        }
    }
}

// Функция для клонирования узла
void cloneExpressionNode(ExpressionNode& dest, const ExpressionNode& src) {
    dest.value = src.value;
    if (src.leftChild) {
        dest.leftChild = std::make_unique<ExpressionNode>(src.leftChild->value);
        cloneExpressionNode(*dest.leftChild, *src.leftChild);
    }
    if (src.rightChild) {
        dest.rightChild = std::make_unique<ExpressionNode>(src.rightChild->value);
        cloneExpressionNode(*dest.rightChild, *src.rightChild);
    }
}

// Функция для замены значения узла
void replaceNodeValue(char target, const ExpressionNode& replacement, ExpressionNode& root) {
    if (root.value == target) {
        cloneExpressionNode(root, replacement);
    } else {
        if (root.leftChild) replaceNodeValue(target, replacement, *root.leftChild);
        if (root.rightChild) replaceNodeValue(target, replacement, *root.rightChild);
    }
}

// Функция для проверки бета-вывода
BetaDerivationResult betaDerivationCheck(const std::vector<std::unique_ptr<ExpressionTree>>& axioms, const ExpressionTree& formula) {
    for (int i = 0; i < axioms.size(); ++i) {
        NodeComparisonResult result = compareExpressionTrees(*axioms[i]->rootNode, *formula.rootNode);
        if (result.resultCode == 2) {
            auto modifiedTree = std::make_unique<ExpressionTree>(axioms[i]->formula);
            replaceNodeValue(result.axiomNode->value, *result.formulaNode, *modifiedTree->rootNode);
            if (compareExpressionNodes(*modifiedTree->rootNode, *formula.rootNode)) {
                std::string subFormula = nodeToString(*result.formulaNode);
                if (subFormula.size() >= 3) {
                    subFormula = subFormula.substr(1, subFormula.size() - 2);
                }
                return {true, i, result.axiomNode->value, subFormula};
            }
        }
    }
    return {};
}

// Функция для проверки Modus Ponens
std::pair<int, int> modusPonensCheck(const std::vector<std::unique_ptr<ExpressionTree>>& axioms, const ExpressionTree& formula) {
    std::vector<size_t> candidates;
    for (size_t i = 0; i < axioms.size(); ++i) {
        if (compareExpressionNodes(*axioms[i]->rootNode->rightChild, *formula.rootNode)) {
            candidates.push_back(i);
        }
    }
    std::pair<int, int> result = {-1, -1};
    for (size_t candidate : candidates) {
        result.first = candidate;
        for (int i = 0; i < axioms.size(); ++i) {
            if (compareExpressionNodes(*axioms[candidate]->rootNode->leftChild, *axioms[i]->rootNode)) {
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
    for (const auto& axiom : axiomsList) {
        axiomsTrees.push_back(createTreeFromFormula(axiom));
    }

    std::string userInput;
    while (true) {
        std::cout << "Введите формулу или 'exit' для завершения программы: ";
        std::getline(std::cin, userInput);
        if (userInput == "exit") break;
        if (userInput.empty()) continue;

        // Разбор и корректировка введенной формулы
        std::string correctInput = parseFormula(userInput);
        if (!correctInput.empty()) {
            auto inputTree = createTreeFromFormula(correctInput);
            bool isEquivalent = false;
            int equivalentIndex = 0;
            for (int i = 0; i < axiomsTrees.size(); ++i) {
                if (compareExpressionNodes(*axiomsTrees[i]->rootNode, *inputTree->rootNode)) {
                    isEquivalent = true;
                    equivalentIndex = i;
                    break;
                }
            }
            if (isEquivalent) {
                std::cout << "Формула " << inputTree->formula << " эквивалентна ";
                if (equivalentIndex < axiomsList.size()) {
                    std::cout << "аксиоме " << axiomsNames[equivalentIndex];
                } else {
                    std::cout << "формуле " << axiomsTrees[equivalentIndex]->formula;
                }
                std::cout << ".\n";
            } else {
                BetaDerivationResult betaResult = betaDerivationCheck(axiomsTrees, *inputTree);
                if (betaResult.isSuccessful) {
                    std::cout << "Формула " << inputTree->formula << " выводима из ";
                    if (betaResult.axiomIdx < axiomsList.size()) {
                        std::cout << "аксиоме " << axiomsNames[betaResult.axiomIdx];
                    } else {
                        std::cout << "формулы " << axiomsTrees[betaResult.axiomIdx]->formula;
                    }
                    std::cout << " с заменой переменной \"" << betaResult.variable << "\" на \"" << betaResult.subFormula << "\".\n";
                    axiomsTrees.push_back(std::move(inputTree));
                    continue;
                } else {
                    std::pair<int, int> mpResult = modusPonensCheck(axiomsTrees, *inputTree);
                    if (mpResult.first != -1 && mpResult.second != -1) {
                        std::cout << "Формула " << inputTree->formula << " выводима из ";
                        if (mpResult.first < axiomsList.size()) {
                            std::cout << "аксиоме " << axiomsNames[mpResult.first];
                        } else {
                            std::cout << "формуле " << axiomsTrees[mpResult.first]->formula;
                        }
                        std::cout << " и ";
                        if (mpResult.second < axiomsList.size()) {
                            std::cout << "аксиоме " << axiomsNames[mpResult.second];
                        } else {
                            std::cout << "формуле " << axiomsTrees[mpResult.second]->formula;
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
