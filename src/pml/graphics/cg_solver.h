#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// ©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤
// Sparse Conjugate Gradient solver for symmetric positive definite systems.
// CSR format. Used by Harmonic UV projection for mesh Laplacian solves.
//
// Shewchuk, "An Introduction to the Conjugate Gradient Method Without
// the Agonizing Pain"
// ©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤

namespace pml {

/// Sparse matrix in Compressed Sparse Row format.
/// Row i: entries at values[row_ptr[i] .. row_ptr[i+1]),
///         columns at col_indices[ same range ].
struct SparseMatrix {
    std::vector<double> values;
    std::vector<int>    col_indices;
    std::vector<int>    row_ptr;  // size = n + 1, row_ptr[n] = nnz
    int                 n = 0;

    [[nodiscard]] int  nnz() const noexcept { return static_cast<int>(values.size()); }
    [[nodiscard]] bool empty() const noexcept { return n == 0; }
};

/// y = A * x  (CSR matrix-vector product)
inline void multiply(const SparseMatrix& A, const std::vector<double>& x,
                     std::vector<double>& y)
{
    auto sz = static_cast<size_t>(A.n);
    y.assign(sz, 0.0);
    for (size_t i = 0; i < sz; ++i) {
        double sum = 0.0;
        for (int j = A.row_ptr[i]; j < A.row_ptr[i + 1]; ++j) {
            sum += A.values[static_cast<size_t>(j)] *
                   x[static_cast<size_t>(A.col_indices[static_cast<size_t>(j)])];
        }
        y[i] = sum;
    }
}

/// Conjugate Gradient solver for symmetric positive definite systems.
struct CGSolver {
    struct Result {
        std::vector<double> x;
        bool                converged = false;
        int                 iterations = 0;
    };

    Result solve(const SparseMatrix& A, const std::vector<double>& b,
                 const std::vector<double>* x0 = nullptr,
                 double tolerance = 1e-10, int max_iterations = 1000) const
    {
        if (A.empty())
            return Result{{}, false, 0};

        auto sz = static_cast<size_t>(A.n);

        // n == 1: direct solve
        if (A.n == 1) {
            double diag = 0.0;
            for (int j = A.row_ptr[0]; j < A.row_ptr[1]; ++j)
                if (A.col_indices[static_cast<size_t>(j)] == 0)
                    diag = A.values[static_cast<size_t>(j)];
            if (std::abs(diag) > 1e-30)
                return Result{{b[0] / diag}, true, 1};
            return Result{{0.0}, false, 1};
        }

        std::vector<double> x(sz, 0.0);
        std::vector<double> r(sz);
        std::vector<double> p(sz);

        // Initial guess
        if (x0 && x0->size() == sz) {
            x = *x0;
            std::vector<double> Ax(sz);
            multiply(A, x, Ax);
            for (size_t i = 0; i < sz; ++i) r[i] = b[i] - Ax[i];
        } else {
            r = b;
        }
        p = r;

        // Convergence threshold = tolerance * max(1, ||b||)
        double norm_b = 0.0;
        for (auto v : b) norm_b += v * v;
        norm_b = std::sqrt(norm_b);
        double threshold = tolerance * std::max(1.0, norm_b);

        double rr = 0.0;
        for (auto v : r) rr += v * v;

        Result result;
        result.x = std::move(x);

        for (int iter = 0; iter < max_iterations; ++iter) {
            if (std::sqrt(rr) < threshold) {
                result.converged = true;
                result.iterations = iter;
                return result;
            }

            // Ap = A * p
            std::vector<double> Ap(sz);
            multiply(A, p, Ap);

            // alpha = dot(r,r) / dot(p, Ap)
            double pAp = 0.0;
            for (size_t i = 0; i < sz; ++i) pAp += p[i] * Ap[i];
            if (std::abs(pAp) < 1e-30) break;  // degenerate direction
            double alpha = rr / pAp;

            // x += alpha * p;  r -= alpha * Ap;  compute new rr
            double rr_new = 0.0;
            for (size_t i = 0; i < sz; ++i) {
                result.x[i] += alpha * p[i];
                double ri = r[i] - alpha * Ap[i];
                rr_new += ri * ri;
                r[i] = ri;
            }

            // beta = dot(r_new, r_new) / dot(r, r)
            double beta = (rr > 0.0) ? rr_new / rr : 0.0;
            for (size_t i = 0; i < sz; ++i)
                p[i] = r[i] + beta * p[i];

            rr = rr_new;
            result.iterations = iter + 1;

            // Final convergence check (after last iteration)
            if (iter == max_iterations - 1 && std::sqrt(rr) < threshold)
                result.converged = true;
        }

        return result;
    }
};

// ©¤©¤ Self-test helpers ©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤

/// Self-test: 3¡Á3 diagonal system -> expects x = [1, 1, 1]
inline bool test_cg_solver_diagonal_3x3()
{
    SparseMatrix A{{2.0, 3.0, 4.0}, {0, 1, 2}, {0, 1, 2, 3}, 3};
    auto r = CGSolver{}.solve(A, {2.0, 3.0, 4.0}, nullptr, 1e-12, 100);
    return r.converged && std::abs(r.x[0] - 1.0) < 1e-9
                      && std::abs(r.x[1] - 1.0) < 1e-9
                      && std::abs(r.x[2] - 1.0) < 1e-9;
}

/// Self-test: 5-point Laplacian on 4x4 grid -> CG converges, residual < 1e-6
inline bool test_cg_solver_laplacian_4x4()
{
    const int N = 4, n = N * N;
    SparseMatrix A;
    A.n = n;
    A.row_ptr.resize(static_cast<size_t>(n) + 1, 0);
    int nnz = 0;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int row = i * N + j;
            A.row_ptr[static_cast<size_t>(row)] = nnz;
            auto push = [&](int col, double val) {
                A.col_indices.push_back(col);
                A.values.push_back(val);
                ++nnz;
            };
            push(row, 4.0);                             // diagonal
            if (j > 0)     push(i * N + (j - 1), -1.0); // left
            if (j < N - 1) push(i * N + (j + 1), -1.0); // right
            if (i > 0)     push((i - 1) * N + j, -1.0); // top
            if (i < N - 1) push((i + 1) * N + j, -1.0); // bottom
        }
    }
    A.row_ptr[static_cast<size_t>(n)] = nnz;

    std::vector<double> b(static_cast<size_t>(n), 0.0);
    for (int i = 1; i < N - 1; ++i)
        for (int j = 1; j < N - 1; ++j)
            b[static_cast<size_t>(i * N + j)] = 1.0;

    auto r = CGSolver{}.solve(A, b, nullptr, 1e-8, 1000);
    if (!r.converged) return false;

    // Verify A*x = b
    std::vector<double> Ax(static_cast<size_t>(n));
    multiply(A, r.x, Ax);
    double max_res = 0.0;
    for (size_t k = 0; k < static_cast<size_t>(n); ++k)
        max_res = std::max(max_res, std::abs(Ax[k] - b[k]));
    return max_res < 1e-6;
}

/// Run all CG solver self-tests. Returns true if all pass.
inline bool test_cg_solver_all()
{
    if (!test_cg_solver_diagonal_3x3()) return false;
    if (!test_cg_solver_laplacian_4x4()) return false;
    // Empty system (n=0)
    {
        auto r = CGSolver{}.solve({}, {});
        if (r.converged || !r.x.empty()) return false;
    }
    // Scalar system (n=1)
    {
        SparseMatrix A{{5.0}, {0}, {0, 1}, 1};
        auto r = CGSolver{}.solve(A, {10.0});
        if (!r.converged || std::abs(r.x[0] - 2.0) > 1e-12) return false;
    }
    // With initial guess
    {
        SparseMatrix A{{2.0, 3.0, 4.0}, {0, 1, 2}, {0, 1, 2, 3}, 3};
        std::vector<double> x0 = {0.5, 0.5, 0.5};
        auto r = CGSolver{}.solve(A, {2.0, 3.0, 4.0}, &x0, 1e-12, 100);
        if (!r.converged) return false;
        if (std::abs(r.x[0] - 1.0) > 1e-9 ||
            std::abs(r.x[1] - 1.0) > 1e-9 ||
            std::abs(r.x[2] - 1.0) > 1e-9) return false;
    }
    return true;
}

}  // namespace pml
