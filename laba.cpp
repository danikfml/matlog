#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <locale>
#include <functional>
#include <fstream>
#include <windows.h>
#include <sstream>

using namespace std;

// Класс узла для абстрактного синтаксического дерева (AST)
class ASTNode {
public:
    wstring value;
    ASTNode* left;
    ASTNode* right;

    ASTNode(wstring val) : value(val), left(nullptr), right(nullptr) {}
    ~ASTNode() {
        delete left;
        delete right;
    }
};

// Класс для работы с формулами
class Formula {
public:
    wstring expression;
    ASTNode* root;

    Formula(wstring expr) : expression(expr), root(nullptr) {
        remove_spaces();
        parse();
    }

    // Удаление пробелов из выражения
    void remove_spaces() {
        expression.erase(remove(expression.begin(), expression.end(), L' '), expression.end());
    }

    // Проверка корректности скобок
    bool validate_parentheses() const {
        int balance = 0;
        for (wchar_t c : expression) {
            if (c == L'(') balance++;
            else if (c == L')') balance--;
            if (balance < 0) return false;
        }
        return balance == 0;
    }

    // Проверка корректности выражения
    bool validate_expression(const wstring& expr) const {
        if (expr.empty()) return false;
        if (expr.size() == 1 && iswalpha(expr[0])) return true;

        int balance = 0;
        for (size_t i = 0; i < expr.size(); ++i) {
            if (expr[i] == L'(') balance++;
            else if (expr[i] == L')') balance--;
            else if (expr[i] == L'-' && i + 1 < expr.size() && expr[i + 1] == L'>' && balance == 0) {
                return validate_expression(expr.substr(0, i)) && validate_expression(expr.substr(i + 2));
            }
        }

        return expr[0] == L'(' && expr.back() == L')' && validate_expression(expr.substr(1, expr.size() - 2));
    }

    // Проверка корректности всей формулы
    bool is_valid() const {
        return validate_parentheses() && validate_expression(expression);
    }

    // Разбор выражения на дерево AST
    ASTNode* parse_expression(const wstring& expr) {
        if (expr.empty()) return nullptr;
        if (expr.size() == 1 && iswalpha(expr[0])) {
            return new ASTNode(expr);
        }

        int balance = 0;
        for (size_t i = 0; i < expr.size(); ++i) {
            if (expr[i] == L'(') balance++;
            else if (expr[i] == L')') balance--;
            else if (expr[i] == L'-' && i + 1 < expr.size() && expr[i + 1] == L'>' && balance == 0) {
                ASTNode* node = new ASTNode(L"->");
                node->left = parse_expression(expr.substr(0, i));
                node->right = parse_expression(expr.substr(i + 2));
                return node;
            }
        }

        if (expr[0] == L'(' && expr.back() == L')') {
            return parse_expression(expr.substr(1, expr.size() - 2));
        }

        return nullptr;
    }

    // Парсинг формулы
    bool parse() {
        root = parse_expression(expression);
        return root != nullptr;
    }

    // Преобразование формулы в строку
    wstring to_string() const {
        return expression;
    }

    // Сравнение структуры двух деревьев AST
    bool compare_structure(ASTNode* other_root) const {
        return compare_trees(root, other_root);
    }

    // Замена переменных в дереве AST
    bool substitute(const ASTNode* node, unordered_map<wstring, wstring>& substitutions) const {
        if (!node) return true;
        if (iswalpha(node->value[0])) {
            if (substitutions.find(node->value) == substitutions.end()) {
                substitutions[node->value] = node->value;
            } else if (substitutions[node->value] != node->value) {
                return false;
            }
        }
        return substitute(node->left, substitutions) && substitute(node->right, substitutions);
    }

    // Сравнение структуры формулы с аксиомой
    bool matches_axiom(ASTNode* axiom_root, unordered_map<wstring, wstring>& substitutions) const {
        return match_structure(root, axiom_root, substitutions);
    }

    // Сравнение структуры формулы с другой формулой
    bool matches_formula(ASTNode* formula_root, unordered_map<wstring, wstring>& substitutions) const {
        return match_structure(root, formula_root, substitutions);
    }

private:
    // Сравнение двух деревьев AST
    bool compare_trees(ASTNode* a, ASTNode* b) const {
        if (!a && !b) return true;
        if (!a || !b) return false;
        if (a->value != b->value) return false;
        return compare_trees(a->left, b->left) && compare_trees(a->right, b->right);
    }

    // Сопоставление структуры двух деревьев AST
    bool match_structure(ASTNode* node, ASTNode* axiom_node, unordered_map<wstring, wstring>& substitutions) const {
        if (!node && !axiom_node) return true;
        if (!node || !axiom_node) return false;
        if (iswalpha(axiom_node->value[0])) {
            if (substitutions.find(axiom_node->value) == substitutions.end()) {
                substitutions[axiom_node->value] = node->value;
            } else if (substitutions[axiom_node->value] != node->value) {
                return false;
            }
        } else if (node->value != axiom_node->value) {
            return false;
        }
        return match_structure(node->left, axiom_node->left, substitutions) && match_structure(node->right, axiom_node->right, substitutions);
    }
};

// Проверка, является ли символ допустимым в выражении
bool is_valid_character(wchar_t ch) {
    return iswalpha(ch) || ch == L'(' || ch == L')' || ch == L'-' || ch == L'>';
}

// Проверка, является ли выражение корректной формулой
bool is_wff(const wstring& expr) {
    int balance = 0;
    for (size_t i = 0; i < expr.size(); ++i) {
        if (expr[i] == L'(') balance++;
        else if (expr[i] == L')') balance--;
        if (balance < 0) return false;
        if (!is_valid_character(expr[i])) return false;
    }
    return balance == 0;
}

// Проверка, содержит ли выражение недопустимые символы
bool has_invalid_characters(const wstring& expr) {
    return any_of(expr.begin(), expr.end(), [](wchar_t ch) { return !is_valid_character(ch); });
}

// Класс для проверки доказуемости формул
class Verifier {
private:
    vector<Formula> proven_formulas;  // Вектор доказанных формул
    vector<pair<function<wstring(wstring, wstring)>, wstring>> axioms_two_param;  // Аксиомы с двумя параметрами
    vector<pair<function<wstring(wstring, wstring, wstring)>, wstring>> axioms_three_param;  // Аксиомы с тремя параметрами
    std::wostringstream result_stream;  // Поток для вывода результатов

public:
    Verifier() {
        // Добавление стандартных аксиом
        axioms_two_param.push_back({[](wstring p, wstring q) { return p + L"->(" + q + L"->" + p + L")"; }, L"A1 (K)"});
        axioms_three_param.push_back({[](wstring s, wstring p, wstring q) { return L"(" + s + L"->(" + p + L"->" + q + L"))->((" + s + L"->" + p + L")->(" + s + L"->" + q + L"))"; }, L"A2 (S)"});
        axioms_two_param.push_back({[](wstring p, wstring f) { return L"((" + p + L"->" + f + L")->" + f + L")->" + p; }, L"A3 (E->)"});
    }

    // Добавление формулы в список доказанных
    bool add_formula(const Formula& formula) {
        if (formula.is_valid()) {
            if (check_previous_formulas(formula)) {
                return true;
            }
            if (check_axioms(formula)) {
                return true;
            }
            if (check_modus_ponens(formula)) {
                return true;
            }

            wcout << L"Формула " << formula.to_string() << L" не выводима." << endl;
            result_stream << L"Формула " << formula.to_string() << L" не выводима." << endl;
            return false;
        } else {
            wcout << L"Формула " << formula.to_string() << L" некорректна." << endl;
            result_stream << L"Формула " << formula.to_string() << L" некорректна." << endl;
            return false;
        }
    }

    // Проверка формулы на соответствие аксиомам
    bool check_axioms(const Formula& formula) {
        for (auto& axiom : axioms_two_param) {
            wstring p = L"p";
            wstring q = L"q";
            wstring axiom_str = axiom.first(p, q);
            Formula axiom_formula(axiom_str);
            unordered_map<wstring, wstring> substitutions;
            if (axiom_formula.is_valid() && formula.matches_axiom(axiom_formula.root, substitutions)) {
                wcout << L"Формула " << formula.to_string() << L" выводима из аксиомы " << axiom.second << L" с подстановкой переменных: ";
                result_stream << L"Формула " << formula.to_string() << L" выводима из аксиомы " << axiom.second << L" с подстановкой переменных: ";
                for (const auto& sub : substitutions) {
                    wcout << sub.first << L" -> " << sub.second << L", ";
                    result_stream << sub.first << L" -> " << sub.second << L", ";
                }
                wcout << endl;
                result_stream << endl;
                proven_formulas.push_back(formula);
                return true;
            }
        }

        for (auto& axiom : axioms_three_param) {
            wstring s = L"s";
            wstring p = L"p";
            wstring q = L"q";
            wstring axiom_str = axiom.first(s, p, q);
            Formula axiom_formula(axiom_str);
            unordered_map<wstring, wstring> substitutions;
            if (axiom_formula.is_valid() && formula.matches_axiom(axiom_formula.root, substitutions)) {
                wcout << L"Формула " << formula.to_string() << L" выводима из аксиомы " << axiom.second << L" с подстановкой переменных: ";
                result_stream << L"Формула " << formula.to_string() << L" выводима из аксиомы " << axiom.second << L" с подстановкой переменных: ";
                for (const auto& sub : substitutions) {
                    wcout << sub.first << L" -> " << sub.second << L", ";
                    result_stream << sub.first << L" -> " << sub.second << L", ";
                }
                wcout << endl;
                result_stream << endl;
                proven_formulas.push_back(formula);
                return true;
            }
        }

        return false;
    }

    // Проверка формулы на соответствие предыдущим доказанным формулам
    bool check_previous_formulas(const Formula& formula) {
        for (const auto& previous_formula : proven_formulas) {
            unordered_map<wstring, wstring> substitutions;
            if (formula.matches_formula(previous_formula.root, substitutions)) {
                wcout << L"Формула " << formula.to_string() << L" выводима из формулы " << previous_formula.to_string() << L" с подстановкой переменных: ";
                result_stream << L"Формула " << formula.to_string() << L" выводима из формулы " << previous_formula.to_string() << L" с подстановкой переменных: ";
                for (const auto& sub : substitutions) {
                    wcout << sub.first << L" -> " << sub.second << L", ";
                    result_stream << sub.first << L" -> " << sub.second << L", ";
                }
                wcout << endl;
                result_stream << endl;
                proven_formulas.push_back(formula);
                return true;
            }
        }
        return false;
    }

    // Проверка формулы на соответствие правилу modus ponens
    bool check_modus_ponens(const Formula& formula) {
        for (const auto& f1 : proven_formulas) {
            wstring antecedent, consequent;
            if (split_implication(f1.expression, antecedent, consequent)) {
                for (const auto& f2 : proven_formulas) {
                    if (f2.expression == antecedent) {
                        if (formula.expression == consequent) {
                            wcout << L"Формула " << formula.to_string() << L" выводима из формул " << f2.to_string() << L" и " << f1.to_string() << L" по правилу modus ponens." << endl;
                            result_stream << L"Формула " << formula.to_string() << L" выводима из формул " << f2.to_string() << L" и " << f1.to_string() << L" по правилу modus ponens." << endl;
                            proven_formulas.push_back(formula);
                            return true;
                        }
                    }
                }
                if (check_modus_ponens_with_axiom(f1, antecedent, formula)) {
                    return true;
                }
            }
        }
        return false;
    }

    // Проверка modus ponens с использованием аксиомы
    bool check_modus_ponens_with_axiom(const Formula& f1, const wstring& antecedent, const Formula& formula) {
        for (auto& axiom : axioms_two_param) {
            wstring p = L"p";
            wstring q = L"q";
            wstring axiom_str = axiom.first(p, q);
            Formula axiom_formula(axiom_str);
            unordered_map<wstring, wstring> substitutions;
            if (axiom_formula.is_valid() && f1.matches_axiom(axiom_formula.root, substitutions)) {
                wcout << L"Формула " << formula.to_string() << L" выводима из формулы " << f1.to_string() << L" и аксиомы " << axiom.second << L" по правилу modus ponens с подстановкой переменных: ";
                result_stream << L"Формула " << formula.to_string() << L" выводима из формулы " << f1.to_string() << L" и аксиомы " << axiom.second << L" по правилу modus ponens с подстановкой переменных: ";
                for (const auto& sub : substitutions) {
                    wcout << sub.first << L" -> " << sub.second << L", ";
                    result_stream << sub.first << L" -> " << sub.second << L", ";
                }
                wcout << endl;
                result_stream << endl;
                proven_formulas.push_back(formula);
                return true;
            }
        }

        for (auto& axiom : axioms_three_param) {
            wstring s = L"s";
            wstring p = L"p";
            wstring q = L"q";
            wstring axiom_str = axiom.first(s, p, q);
            Formula axiom_formula(axiom_str);
            unordered_map<wstring, wstring> substitutions;
            if (axiom_formula.is_valid() && f1.matches_axiom(axiom_formula.root, substitutions)) {
                wcout << L"Формула " << formula.to_string() << L" выводима из формулы " << f1.to_string() << L" и аксиомы " << axiom.second << L" по правилу modus ponens с подстановкой переменных: ";
                result_stream << L"Формула " << formula.to_string() << L" выводима из формулы " << f1.to_string() << L" и аксиомы " << axiom.second << L" по правилу modus ponens с подстановкой переменных: ";
                for (const auto& sub : substitutions) {
                    wcout << sub.first << L" -> " << sub.second << L", ";
                    result_stream << sub.first << L" -> " << sub.second << L", ";
                }
                wcout << endl;
                result_stream << endl;
                proven_formulas.push_back(formula);
                return true;
            }
        }

        return false;
    }

    // Поиск формулы в списке доказанных формул
    bool find_formula(const wstring& expr) const {
        for (const auto& f : proven_formulas) {
            if (f.expression == expr) {
                return true;
            }
        }
        return false;
    }

    // Разделение выражения на антецедент и консеквент
    bool split_implication(const wstring& expr, wstring& antecedent, wstring& consequent) const {
        int balance = 0;
        for (size_t i = 0; i < expr.size(); ++i) {
            if (expr[i] == L'(') balance++;
            else if (expr[i] == L')') balance--;
            else if (expr[i] == L'-' && i + 1 < expr.size() && expr[i + 1] == L'>' && balance == 0) {
                antecedent = expr.substr(0, i);
                consequent = expr.substr(i + 2);
                return true;
            }
        }
        return false;
    }

    // Получение текущей рабочей директории
    std::wstring get_current_working_directory() {
        wchar_t buffer[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, buffer);
        return std::wstring(buffer);
    }

    // Импорт формул из файла
    void import_formulas_from_file(const std::wstring& filename) {
        std::wifstream file(filename.c_str());
        if (!file.is_open()) {
            std::wstring full_path = get_current_working_directory() + L"\\" + filename;
            file.open(full_path.c_str());
            if (!file.is_open()) {
                std::wcout << L"Не удалось открыть файл: " << filename << std::endl;
                result_stream << L"Не удалось открыть файл: " << filename << std::endl;
                return;
            }
        }
        std::wstring line;
        while (getline(file, line)) {
            if (has_invalid_characters(line) || !is_wff(line)) {
                std::wcout << L"Формула " << line << L" некорректна." << std::endl;
                result_stream << L"Формула " << line << L" некорректна." << std::endl;
                continue;
            }
            Formula formula(line);
            add_formula(formula);
        }
    }

    // Экспорт результатов в файл
    void export_results_to_file(const std::wstring& filename) {
        std::wofstream file(filename.c_str());
        if (!file.is_open()) {
            std::wstring full_path = get_current_working_directory() + L"\\" + filename;
            file.open(full_path.c_str());
            if (!file.is_open()) {
                std::wcout << L"Не удалось открыть файл: " << filename << std::endl;
                return;
            }
        }
        file << result_stream.str();
    }

    // Добавление новой аксиомы
    void add_axiom(const wstring& axiom) {
        wstring p = L"p";
        wstring q = L"q";
        if (axiom.find(L"s") != std::wstring::npos) {
            axioms_three_param.push_back({[axiom](wstring s, wstring p, wstring q) { return axiom; }, L"Пользовательская аксиома"});
        } else {
            axioms_two_param.push_back({[axiom](wstring p, wstring q) { return axiom; }, L"Пользовательская аксиома"});
        }
        wcout << L"Аксиома добавлена." << endl;
    }

    // Удаление аксиомы
    void remove_axiom(const wstring& axiom) {
        auto it = remove_if(axioms_two_param.begin(), axioms_two_param.end(),
                            [&axiom](const auto& pair) { return pair.first(L"p", L"q") == axiom; });
        if (it != axioms_two_param.end()) {
            axioms_two_param.erase(it, axioms_two_param.end());
            wcout << L"Аксиома удалена." << endl;
            return;
        }

        auto it2 = remove_if(axioms_three_param.begin(), axioms_three_param.end(),
                             [&axiom](const auto& pair) { return pair.first(L"s", L"p", L"q") == axiom; });
        if (it2 != axioms_three_param.end()) {
            axioms_three_param.erase(it2, axioms_three_param.end());
            wcout << L"Аксиома удалена." << endl;
            return;
        }

        wcout << L"Аксиома не найдена." << endl;
    }

    // Просмотр всех аксиом
    void view_axioms() const {
        wcout << L"Аксиомы с двумя параметрами:" << endl;
        for (const auto& axiom : axioms_two_param) {
            wcout << axiom.first(L"p", L"q") << L" : " << axiom.second << endl;
        }
        wcout << L"Аксиомы с тремя параметрами:" << endl;
        for (const auto& axiom : axioms_three_param) {
            wcout << axiom.first(L"s", L"p", L"q") << L" : " << axiom.second << endl;
        }
    }
};

int main() {
    setlocale(LC_ALL, "Russian");
    Verifier verifier;

    while (true) {
        wcout << L"Меню:\n";
        wcout << L"1. Ввести формулу\n";
        wcout << L"2. Импорт формул из файла\n";
        wcout << L"3. Экспорт результатов в файл\n";
        wcout << L"4. Добавить аксиому\n";
        wcout << L"5. Удалить аксиому\n";
        wcout << L"6. Просмотреть аксиомы\n";
        wcout << L"0. Выход\n";
        wcout << L"Выберите опцию: ";

        int choice;
        wcin >> choice;
        wcin.ignore();

        switch (choice) {
            case 0:
                return 0;
            case 1: {
                wstring input;
                while (true) {
                    wcout << L"Введите формулу (или exit для выхода): ";
                    getline(wcin, input);
                    if (input == L"exit") break;
                    if (has_invalid_characters(input) || !is_wff(input)) {
                        wcout << L"Формула " << input << L" некорректна." << endl;
                    } else {
                        Formula formula(input);
                        verifier.add_formula(formula);
                    }
                }
                break;
            }
            case 2: {
                wstring filename;
                wcout << L"Введите имя файла для импорта формул: ";
                getline(wcin, filename);
                verifier.import_formulas_from_file(filename);
                break;
            }
            case 3: {
                wstring filename;
                wcout << L"Введите имя файла для экспорта результатов: ";
                getline(wcin, filename);
                verifier.export_results_to_file(filename);
                break;
            }
            case 4: {
                wstring axiom;
                wcout << L"Введите аксиому: ";
                getline(wcin, axiom);
                verifier.add_axiom(axiom);
                break;
            }
            case 5: {
                wstring axiom;
                wcout << L"Введите аксиому для удаления: ";
                getline(wcin, axiom);
                verifier.remove_axiom(axiom);
                break;
            }
            case 6: {
                verifier.view_axioms();
                break;
            }
            default:
                wcout << L"Неверная опция. Попробуйте снова." << endl;
                break;
        }
    }
}
