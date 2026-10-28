#include "bond/analytics.hpp"

#include <charconv>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void help() {
    std::cout <<
        "Fixed-rate bullet-bond analytics, valued on a coupon payment date.\n"
        "Usage: bond_analytics [option value ...]\n\n"
        "  --face NUMBER          Principal, positive (default 1000)\n"
        "  --coupon RATE          Annual coupon as a decimal (default 0.05)\n"
        "  --periods INTEGER      Remaining coupon periods, 1..10000 (default 10)\n"
        "  --frequency INTEGER    Payments per year, 1..365 (default 2)\n"
        "  --yield RATE           Nominal annual yield (default 0.04)\n"
        "  --method NAME          direct or horner (default horner)\n"
        "  --target-price NUMBER  Also solve the yield implied by this price\n"
        "  --lower RATE           Solver lower yield (default -0.5)\n"
        "  --upper RATE           Solver upper yield (default 1.0)\n"
        "  --help                 Show this help; use it alone\n\n"
        "All rates use decimals: 0.05 means 5%, not 5.\n"
        "The yield is compounded at --frequency and must exceed -frequency.\n"
        "Maturity in years is periods/frequency. Cash flows have equal periods.\n"
        "Clean and dirty prices agree here: accrued interest is zero.\n"
        "No day-count conventions, settlement dates, calls or default are modeled.\n"
        "Output is CSV on standard output; errors use standard error and exit 1.\n";
}

bool known_option(const std::string& option) {
    return option == "--face" || option == "--coupon" || option == "--periods" ||
           option == "--frequency" || option == "--yield" || option == "--method" ||
           option == "--target-price" || option == "--lower" || option == "--upper";
}

double number(const std::string& text, const std::string& name) {
    std::istringstream input(text);
    input.imbue(std::locale::classic());
    input >> std::noskipws;
    double result = 0.0;
    if (!(input >> result) || input.peek() != std::char_traits<char>::eof() ||
        !std::isfinite(result)) {
        throw std::invalid_argument(name + " requires a finite decimal number");
    }
    return result;
}

std::size_t positive_integer(const std::string& text, const std::string& name) {
    if (text.empty()) {
        throw std::invalid_argument(name + " requires a positive integer");
    }
    for (char digit : text) {
        if (digit < '0' || digit > '9') {
            throw std::invalid_argument(name + " requires decimal digits only");
        }
    }
    std::size_t result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || result == 0) {
        throw std::invalid_argument(name + " is outside the positive integer range");
    }
    return result;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            help();
            return 0;
        }
        std::map<std::string, std::string> options;
        for (int index = 1; index < argc; index += 2) {
            const std::string option(argv[index]);
            if (!known_option(option)) {
                throw std::invalid_argument("unknown option: " + option);
            }
            if (index + 1 == argc) {
                throw std::invalid_argument("missing value for " + option);
            }
            if (!options.emplace(option, argv[index + 1]).second) {
                throw std::invalid_argument("duplicate option: " + option);
            }
        }
        auto numeric_option = [&options](const char* key, double fallback) {
            const auto found = options.find(key);
            return found == options.end() ? fallback : number(found->second, key);
        };
        bond::Bond contract;
        contract.face_value = numeric_option("--face", contract.face_value);
        contract.coupon_rate = numeric_option("--coupon", contract.coupon_rate);
        if (options.count("--periods") != 0) {
            contract.periods = positive_integer(options.at("--periods"), "--periods");
        }
        if (options.count("--frequency") != 0) {
            const auto frequency = positive_integer(options.at("--frequency"), "--frequency");
            if (frequency > std::numeric_limits<unsigned>::max()) {
                throw std::invalid_argument("--frequency exceeds the unsigned integer range");
            }
            contract.payments_per_year = static_cast<unsigned>(frequency);
        }
        bond::Method method = bond::Method::horner;
        std::string method_name = "horner";
        if (options.count("--method") != 0) {
            method_name = options.at("--method");
            if (method_name == "direct") {
                method = bond::Method::direct;
            } else if (method_name != "horner") {
                throw std::invalid_argument("--method must be direct or horner");
            }
        }
        if (options.count("--target-price") == 0 &&
            (options.count("--lower") != 0 || options.count("--upper") != 0)) {
            throw std::invalid_argument("--lower and --upper require --target-price");
        }
        const double annual_yield = numeric_option("--yield", 0.04);
        const bond::Metrics metrics = bond::analyze(contract, annual_yield, method);
        double target_price = 0.0;
        double implied_yield = 0.0;
        if (options.count("--target-price") != 0) {
            target_price = numeric_option("--target-price", 0.0);
            implied_yield = bond::yield_to_maturity(contract, target_price,
                numeric_option("--lower", -0.5), numeric_option("--upper", 1.0));
        }
        std::cout.imbue(std::locale::classic());
        std::cout << std::setprecision(17)
                  << "metric,value\nmethod," << method_name
                  << "\nprice," << metrics.price
                  << "\nmacaulay_duration," << metrics.macaulay_duration
                  << "\nmodified_duration," << metrics.modified_duration
                  << "\nconvexity," << metrics.convexity << '\n';
        if (options.count("--target-price") != 0) {
            std::cout << "target_price," << target_price
                      << "\nimplied_yield," << implied_yield << '\n';
        }
        if (!std::cout) {
            throw std::runtime_error("failed to write CSV output");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\nUse --help for usage.\n";
        return 1;
    }
}
