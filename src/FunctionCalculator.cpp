#include "FunctionCalculator.h"

#include "Sin.h"
#include "Ln.h"
#include "Poly.h"
#include "Mul.h"
#include "Add.h"
#include "Comp.h"
#include "Log.h"
#include "NotDigitException.h"

#include <istream>
#include <ostream>
#include <iomanip>
#include <sstream>


FunctionCalculator::FunctionCalculator(std::istream& istr, std::ostream& ostr)
    : m_maxSize(300), m_actions(createActions()), m_functions(createFunctions()), m_istr(istr), m_ostr(ostr)
{
}

void FunctionCalculator::run()
{
        firstResize();
    
    m_ostr << std::setprecision(2) << std::fixed;
    do{
        m_ostr << '\n';
        printFunctions();
        m_ostr << "Enter command ('help' for the list of available commands): ";
        const auto action = readAction();
        runAction(action);
    } while (m_running);
}

void FunctionCalculator::eval()
{
    if (auto i = readFunctionIndex(); i)
    {
        auto x = 0.;
        m_istr >> x;
        excLetter();
        auto sstr = std::ostringstream();
        sstr << std::setprecision(2) << std::fixed << x;
        m_ostr << m_functions[*i]->to_string(sstr.str())
            << " = "
            << (*m_functions[*i])(x)
            << '\n';
    }
}

void FunctionCalculator::poly()
{
    if (m_functions.size() < m_maxSize)
    {
        auto n = 0;
        m_istr >> n;
        excLetter(); // to degree of poly
        excPositive(n);
        auto coeffs = std::vector<double>(n);
        for (auto& coeff : coeffs)
        {
            m_istr >> coeff;
            excLetter();
        }
        m_functions.push_back(std::make_shared<Poly>(coeffs));
    }
    else
    {
        throw std::out_of_range("Too much functions\n");
    }
}

void FunctionCalculator::log()
{
    if (m_functions.size() < m_maxSize)
    {
        auto base = 0;
        m_istr >> base;
        if (auto f = readFunctionIndex(); f)
        {
            m_functions.push_back(std::make_shared<Log>(base, m_functions[*f]));
        }
    }
    else
    {
        throw std::out_of_range("Too much functions\n");
    }
}

void FunctionCalculator::del()
{
    if (auto i = readFunctionIndex(); i)
    {
        m_functions.erase(m_functions.begin() + *i);
    }
}

void FunctionCalculator::help()
{
    m_ostr << "The available commands are:\n";
    for (const auto& action : m_actions)
    {
        m_ostr << "* " << action.command << action.description << '\n';
    }
    m_ostr << '\n';
}

void FunctionCalculator::exit()
{
    m_ostr << "Goodbye!\n";
    m_running = false;
}

void FunctionCalculator::resize()
{
    int wanted, sure;
    m_istr >> wanted;
    excLetter();
    excRange(2, 100, wanted, "the range between 2-100\n");
    if (wanted < m_functions.size())
    { 
       sure = yesOrNo();
       if(sure == 1)
          m_functions.resize(wanted);
    }
    m_maxSize = wanted;
}

void FunctionCalculator::firstResize()
{
    while (m_maxSize == 300)
    {
        try {
            m_ostr << "please enter number of wanted functions: ";
            resize();
        }
        catch (std::out_of_range& e)
        {
            m_ostr << e.what();
            firstResize();
        }
        catch (NotDigitException& e)
        {
            m_ostr << e.what();
            firstResize();
        }

    }
}

int FunctionCalculator::yesOrNo()
{
    int wanted = 0;
    while (wanted != 1 && wanted != 2)
    {
        try
        {
            m_ostr << "Are you sure ?\nEnter 1 to yes or 2 to no";
            m_istr >> wanted;
            if (!m_istr)
            {
                m_istr.clear();
                m_istr.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                throw NotDigitException();
            }
            if (wanted < 1 || wanted > 2)
            {
                throw std::out_of_range("wanted number is not 1/2\n");
            }
        }
        catch (const std::out_of_range& e)
        {
            m_ostr << e.what();
        }
        catch (NotDigitException& e)
        {
            m_ostr << e.what();
        }
      
    }
    return wanted;
}

void FunctionCalculator::excRange(const int start, const int end, const int wanted, const std::string error)const
{
    if (wanted < start || wanted > end)
        throw std::out_of_range(error);
}

void FunctionCalculator::excPositive(const int wanted)const
{
    if (wanted < 0)
        throw std::out_of_range("This is negative coeff\n");
}

void FunctionCalculator::excLetter()const
{
    if (!m_istr)
    {
        m_istr.clear();
        m_istr.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        throw NotDigitException();
    }
}


void FunctionCalculator::printFunctions() const
{
    m_ostr << "List of available gates:\n";
    for (decltype(m_functions.size()) i = 0; i < m_functions.size(); ++i)
    {
        m_ostr << i << ".\t" << m_functions[i]->to_string("x") << '\n';
    }
    m_ostr << '\n';
}

std::optional<int> FunctionCalculator::readFunctionIndex() const
{
    auto i = 0;
    m_istr >> i;
    excLetter();
    excPositive(i);
    excRange(0, m_functions.size(), i, "Function doesn't exist\n");
   
    return i;
}

FunctionCalculator::Action FunctionCalculator::readAction() const
{
    auto action = std::string();
    m_istr >> action;

    for (decltype(m_actions.size()) i = 0; i < m_actions.size(); ++i)
    {
        if (action == m_actions[i].command)
        {
            return m_actions[i].action;
        }
    }

    return Action::Invalid;
}

void FunctionCalculator::runAction(Action action)
{
    try {
        switch (action)
        {
        default:
            m_ostr << "Unknown enum entry used!\n";
            break;

        case Action::Invalid:
            m_ostr << "Command not found\n";
            break;

        case Action::Eval: eval();             break;
        case Action::Poly: poly();             break;
        case Action::Mul:  binaryFunc<Mul>();  break;
        case Action::Add:  binaryFunc<Add>();  break;
        case Action::Comp: binaryFunc<Comp>(); break;
        case Action::Log:  log();              break;
        case Action::Del:  del();              break;
        case Action::Help: help();             break;
        case Action::Exit: exit();             break;
        case Action::Resize: resize();         break;

        }
    
    }
    catch (NotDigitException& e)
    {
        m_ostr << e.what();      
    }
    catch (std::out_of_range& e)
    {
        m_ostr << e.what();
    }

}

FunctionCalculator::ActionMap FunctionCalculator::createActions()
{
    return ActionMap
    {
        {
            "eval",
            "(uate) num x - compute the result of function #num on x",
            Action::Eval
        },
        {
            "poly",
            "(nomial) N c_0 c_1 ... c_(N-1) - creates a polynomial with N coefficients",
            Action::Poly
        },
        {
            "mul",
            "(tiply) num1 num2 - Creates a function that is the multiplication of "
            "function #num1 and function #num2",
            Action::Mul
        },
        {
            "add",
            " num1 num2 - Creates a function that is the sum of function #num1 and "
            "function #num2",
            Action::Add
        },
        {
            "comp",
            "(osite) num1 num2 - creates a function that is the composition of "
            "function #num1 and function #num2",
            Action::Comp
        },
        {
            "log",
            " N num - create a function that is the log_N of function #num",
            Action::Log
        },
        {
            "del",
            "(ete) num - delete function #num from the function list",
            Action::Del
        },
        {
            "help",
            " - print this command list",
            Action::Help
        },
        {
            "exit",
            " - exit the program",
            Action::Exit
        },
        {
            "resize",
            " - resize number of functions (valid : 2-100)",
            Action::Resize
        }
    };
}

FunctionCalculator::FunctionList FunctionCalculator::createFunctions()
{
    return FunctionList
    {
        std::make_shared<Sin>(),
        std::make_shared<Ln>()
    };
}