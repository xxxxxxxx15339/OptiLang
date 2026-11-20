
#include "core/glpk_solver.hpp"

#include <glpk.h>

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace optilang {
namespace {

// Automatically free GLPK memory.
using ProblemPtr =
    std::unique_ptr<glp_prob, decltype(&glp_delete_prob)>;

// GLPK uses int indices, starting at 1.
int asIndex(std::size_t index) {
    if (index > static_cast<std::size_t>(
            std::numeric_limits<int>::max())) {
        throw std::runtime_error(
            "optimization model exceeds GLPK index limits"
        );
    }

    return static_cast<int>(index);
}

void requireFinite(
    double value,
    const std::string& context
) {
    if (!std::isfinite(value)) {
        throw std::runtime_error(
            "non-finite value in " + context
        );
    }
}

SolveResult notSolved(
    SolveStatus status,
    std::string message,
    bool mip = false
) {
    SolveResult result;

    result.status = status;
    result.usedIntegerOptimizer = mip;
    result.message = std::move(message);

    return result;
}

// Configure a variable's domain.
// Returns false when bounds are contradictory.
bool configureBounds(
    glp_prob* lp,
    int column,
    const IRVariable& var
) {
    auto lower = var.lowerBound;
    auto upper = var.upperBound;

    if (lower) {
        requireFinite(
            *lower,
            "lower bound for '" + var.name + "'"
        );
    }

    if (upper) {
        requireFinite(
            *upper,
            "upper bound for '" + var.name + "'"
        );
    }

    // For integer variables, tighten fractional bounds.
    if (var.type == VariableType::Integer) {
        if (lower) lower = std::ceil(*lower);
        if (upper) upper = std::floor(*upper);
    }

    if (lower && upper && *lower > *upper) {
        return false;
    }

    if (lower && upper && *lower == *upper) {
        glp_set_col_bnds(
            lp, column, GLP_FX, *lower, *upper
        );
    }
    else if (lower && upper) {
        glp_set_col_bnds(
            lp, column, GLP_DB, *lower, *upper
        );
    }
    else if (lower) {
        glp_set_col_bnds(
            lp, column, GLP_LO, *lower, 0.0
        );
    }
    else if (upper) {
        glp_set_col_bnds(
            lp, column, GLP_UP, 0.0, *upper
        );
    }
    else {
        // Important: OptiLang variables are free by default.
        glp_set_col_bnds(
            lp, column, GLP_FR, 0.0, 0.0
        );
    }

    glp_set_col_kind(
        lp,
        column,
        var.type == VariableType::Integer
            ? GLP_IV
            : GLP_CV
    );

    return true;
}

// Retrieve the values after optimization.
void readSolution(
    glp_prob* lp,
    const LinearIR& model,
    bool mip,
    SolveResult& result
) {
    result.objectiveValue =
        mip ? glp_mip_obj_val(lp)
            : glp_get_obj_val(lp);

    requireFinite(
        result.objectiveValue,
        "solver objective"
    );

    for (std::size_t i = 0;
         i < model.variables.size(); ++i) {

        const int column = asIndex(i + 1);

        const double value =
            mip ? glp_mip_col_val(lp, column)
                : glp_get_col_prim(lp, column);

        const std::string& name =
            model.variables[i].name;

        requireFinite(
            value,
            "solution variable '" + name + "'"
        );

        result.variableValues.emplace(name, value);
    }
}

} // anonymous namespace

// ===========================================
// MAIN SOLVER FUNCTION
// ===========================================

SolveResult solveWithGlpk(const LinearIR& model) {

    if (!model.solveRequested || !model.objective) {
        throw std::invalid_argument(
            "GLPK requires solve; and an objective"
        );
    }

    if (model.variables.empty()) {
        throw std::invalid_argument(
            "GLPK model requires at least one variable"
        );
    }

    asIndex(model.variables.size());
    asIndex(model.constraints.size());

    // Detect whether integer optimization is required.
    bool mip = false;

    for (const auto& variable : model.variables) {
        if (variable.type == VariableType::Integer) {
            mip = true;
            break;
        }
    }

    // Create GLPK problem object.
    ProblemPtr lp(
        glp_create_prob(),
        &glp_delete_prob
    );

    if (!lp) {
        throw std::runtime_error(
            "glp_create_prob failed"
        );
    }

    // =======================================
    // STEP 1: OBJECTIVE DIRECTION
    // =======================================

    glp_set_obj_dir(
        lp.get(),
        model.objective->sense == ObjectiveSense::Maximize
            ? GLP_MAX
            : GLP_MIN
    );

    // =======================================
    // STEP 2: VARIABLES / COLUMNS
    // =======================================

    glp_add_cols(
        lp.get(),
        asIndex(model.variables.size())
    );

    std::unordered_map<std::string, int> columns;
    columns.reserve(model.variables.size());

    for (std::size_t i = 0;
         i < model.variables.size(); ++i) {

        const IRVariable& variable = model.variables[i];
        const int column = asIndex(i + 1);

        if (variable.name.empty() ||
            !columns.emplace(variable.name, column).second) {

            throw std::runtime_error(
                "invalid or duplicate variable '" +
                variable.name + "'"
            );
        }

        glp_set_col_name(
            lp.get(),
            column,
            variable.name.c_str()
        );

        if (!configureBounds(
                lp.get(), column, variable)) {

            return notSolved(
                SolveStatus::Infeasible,
                "contradictory bounds for '" +
                variable.name + "'",
                mip
            );
        }
    }

    // =======================================
    // STEP 3: OBJECTIVE COEFFICIENTS
    // =======================================

    requireFinite(
        model.objective->constant,
        "objective constant"
    );

    // Column 0 represents the constant objective term.
    glp_set_obj_coef(
        lp.get(),
        0,
        model.objective->constant
    );

    for (const auto& [name, coefficient] :
         model.objective->coefficients) {

        const auto it = columns.find(name);

        if (it == columns.end()) {
            throw std::runtime_error(
                "objective refers to unknown variable '" +
                name + "'"
            );
        }

        requireFinite(
            coefficient,
            "objective coefficient"
        );

        glp_set_obj_coef(
            lp.get(),
            it->second,
            coefficient
        );
    }

    // =======================================
    // STEP 4: CONSTRAINTS / ROWS
    // =======================================

    const int rows = asIndex(model.constraints.size());

    // GLPK must not be asked to add zero rows.
    if (rows > 0) {
        glp_add_rows(lp.get(), rows);
    }

    // GLPK sparse matrix arrays are 1-indexed.
    std::vector<int> ia(1, 0);
    std::vector<int> ja(1, 0);
    std::vector<double> ar(1, 0.0);

    for (std::size_t i = 0;
         i < model.constraints.size(); ++i) {

        const IRConstraint& constraint =
            model.constraints[i];

        const int row = asIndex(i + 1);

        requireFinite(
            constraint.rhs,
            "constraint RHS"
        );

        if (!constraint.name.empty()) {
            glp_set_row_name(
                lp.get(),
                row,
                constraint.name.c_str()
            );
        }

        switch (constraint.relation) {

            case Relation::LessEqual:
                glp_set_row_bnds(
                    lp.get(),
                    row,
                    GLP_UP,
                    0.0,
                    constraint.rhs
                );
                break;

            case Relation::GreaterEqual:
                glp_set_row_bnds(
                    lp.get(),
                    row,
                    GLP_LO,
                    constraint.rhs,
                    0.0
                );
                break;

            case Relation::Equal:
                glp_set_row_bnds(
                    lp.get(),
                    row,
                    GLP_FX,
                    constraint.rhs,
                    constraint.rhs
                );
                break;

            default:
                throw std::runtime_error(
                    "strict inequality reached GLPK backend"
                );
        }

        // Insert sparse matrix coefficients.
        for (const auto& [name, coefficient] :
             constraint.coefficients) {

            const auto it = columns.find(name);

            if (it == columns.end()) {
                throw std::runtime_error(
                    "constraint refers to unknown variable '" +
                    name + "'"
                );
            }

            requireFinite(
                coefficient,
                "constraint coefficient"
            );

            if (coefficient == 0.0)
                continue;

            asIndex(ar.size());

            ia.push_back(row);
            ja.push_back(it->second);
            ar.push_back(coefficient);
        }
    }

    // Load matrix only when it has nonzero elements.
    if (ar.size() > 1) {
        glp_load_matrix(
            lp.get(),
            asIndex(ar.size() - 1),
            ia.data(),
            ja.data(),
            ar.data()
        );
    }

    // =======================================
    // STEP 5: RUN OPTIMIZATION
    // =======================================

    SolveResult result;
    result.usedIntegerOptimizer = mip;

    int error = 0;
    int status = GLP_UNDEF;

    if (mip) {
        // MILP: branch-and-cut.
        glp_iocp options;
        glp_init_iocp(&options);

        options.msg_lev = GLP_MSG_OFF;
        options.presolve = GLP_ON;

        error = glp_intopt(
            lp.get(),
            &options
        );

        if (error == GLP_ENOPFS) {
            return notSolved(
                SolveStatus::Infeasible,
                "LP relaxation is infeasible",
                true
            );
        }

        if (error == GLP_ENODFS) {
            return notSolved(
                SolveStatus::Unknown,
                "LP relaxation is unbounded; "
                "MIP status not certified",
                true
            );
        }

        status = glp_mip_status(lp.get());

        if (status == GLP_OPT)
            result.status = SolveStatus::Optimal;
        else if (status == GLP_FEAS)
            result.status = SolveStatus::Feasible;
        else if (status == GLP_NOFEAS)
            result.status = SolveStatus::Infeasible;

    } else {
        // LP: simplex.
        glp_smcp options;
        glp_init_smcp(&options);

        options.msg_lev = GLP_MSG_OFF;

        error = glp_simplex(
            lp.get(),
            &options
        );

        if (error == GLP_ENOPFS) {
            return notSolved(
                SolveStatus::Infeasible,
                "LP has no feasible solution"
            );
        }

        if (error == GLP_ENODFS) {
            return notSolved(
                SolveStatus::Unknown,
                "LP has no dual feasible solution"
            );
        }

        status = glp_get_status(lp.get());

        if (status == GLP_OPT)
            result.status = SolveStatus::Optimal;
        else if (status == GLP_FEAS)
            result.status = SolveStatus::Feasible;
        else if (status == GLP_NOFEAS)
            result.status = SolveStatus::Infeasible;
        else if (status == GLP_UNBND)
            result.status = SolveStatus::Unbounded;
    }

    if (error != 0) {
        result.message =
            "GLPK returned error code " +
            std::to_string(error);
    }

    // Only retrieve solution values when a feasible
    // solution actually exists.
    if (result.status == SolveStatus::Optimal ||
        result.status == SolveStatus::Feasible) {

        readSolution(lp.get(), model, mip, result);

    } else if (result.message.empty()) {

        result.message =
            "no optimal/feasible solution available";
    }

    return result;
}

// ===========================================
// STATUS DISPLAY
// ===========================================

const char* solveStatusName(SolveStatus status) {
    switch (status) {
        case SolveStatus::Optimal:
            return "OPTIMAL";

        case SolveStatus::Feasible:
            return "FEASIBLE (not proven optimal)";

        case SolveStatus::Infeasible:
            return "INFEASIBLE";

        case SolveStatus::Unbounded:
            return "UNBOUNDED";

        case SolveStatus::Unknown:
            return "UNKNOWN";
    }

    return "UNKNOWN";
}

// ===========================================
// OUTPUT RESULTS
// ===========================================

void printSolveResult(
    const LinearIR& model,
    const SolveResult& result,
    std::ostream& out
) {
    out << "\n===== GLPK Solution =====\n";

    out << "Method: "
        << (result.usedIntegerOptimizer ? "MILP" : "LP")
        << '\n';

    out << "Status: "
        << solveStatusName(result.status)
        << '\n';

    if (!result.message.empty()) {
        out << "Details: "
            << result.message << '\n';
    }

    if (result.status != SolveStatus::Optimal &&
        result.status != SolveStatus::Feasible) {
        return;
    }

    auto show = [&](const std::string& name) {

        if (model.objective &&
            name == model.objective->name) {

            out << name << " = "
                << result.objectiveValue << '\n';

        } else {
            const auto it =
                result.variableValues.find(name);

            if (it != result.variableValues.end()) {
                out << name << " = "
                    << it->second << '\n';
            }
        }
    };

    // If print was requested, honor that list.
    if (model.printNames) {
        for (const auto& name : *model.printNames) {
            show(name);
        }
    } else {
        // Otherwise print all results.
        if (model.objective) {
            show(model.objective->name);
        }

        for (const auto& variable : model.variables) {
            show(variable.name);
        }
    }
}

} // namespace optilang

