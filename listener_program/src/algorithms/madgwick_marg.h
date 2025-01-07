#pragma once
#include "../pch.h"

/**
 * @brief Madgwick MARG filter class using Eigen for math operations.
 */
class MadgwickMARG
{
   public:
    /// Time step between updates (seconds)
    float samplePeriod;
    /// Algorithm gain (commonly denoted \beta)
    float beta;
    /// Magnetometer drift-correction gain (commonly denoted \zeta)
    float zeta;
    /// Current orientation as a quaternion (w, x, y, z)
    Eigen::Quaternionf q;

   public:
    /**
     * @brief Construct a new MadgwickMARG object.
     * @param samplePeriod_ Filter update period in seconds.
     * @param beta_ Algorithm gain.
     * @param zeta_ Magnetometer drift-correction gain.
     */
    MadgwickMARG(float samplePeriod_ = 0.037f, float beta_ = 0.1f, float zeta_ = 0.01f)
        : samplePeriod(samplePeriod_),
          beta(beta_),
          zeta(zeta_),
          q(0.875f, .017f, 0.007f, -0.48f)  // identity quaternion
    {
    }

    /**
     * @brief Perform one update step with data from an AHRS sensor array.
     *
     * @param gyroscope     3D gyroscope data (rad/s).
     * @param accelerometer 3D accelerometer data (any unit, internally normalized).
     * @param magnetometer  3D magnetometer data (any unit, internally normalized).
     */
    void update(const Eigen::Vector3f& gyroscope, const Eigen::Vector3f& accelerometer,
                const Eigen::Vector3f& magnetometer)
    {
        // Copy sensor data for local manipulation
        Eigen::Vector3f g = gyroscope;
        Eigen::Vector3f a = accelerometer;
        Eigen::Vector3f m = magnetometer;

        //-------------------------------------------
        // 1. Normalize accelerometer and magnetometer
        //-------------------------------------------
        float aNorm = a.norm();
        if (aNorm < 1e-12f)
        {
            std::cerr << "Warning: Accelerometer norm is zero." << std::endl;
            return;
        }
        a /= aNorm;

        float mNorm = m.norm();
        if (mNorm < 1e-12f)
        {
            std::cerr << "Warning: Magnetometer norm is zero." << std::endl;
            return;
        }
        m /= mNorm;

        // Current orientation (Eigen stores w, x, y, z in q.w(), q.x(), ...)
        float q0 = q.w();
        float q1 = q.x();
        float q2 = q.y();
        float q3 = q.z();

        //----------------------------------------------------------
        // 2. Compute reference direction of Earth's magnetic field
        //----------------------------------------------------------
        // h = q * (0, mx, my, mz) * q.conjugate()
        Eigen::Quaternionf magQ(0.f, m.x(), m.y(), m.z());
        Eigen::Quaternionf h = q * magQ * q.conjugate();

        // b = [0, sqrt(hx^2 + hy^2), 0, hz]
        float hx = h.x(), hy = h.y(), hz = h.z();
        float bx = std::sqrt(hx * hx + hy * hy);
        float bz = hz;

        //--------------------------------------------------
        // 3. Compute the objective function f (6x1 vector)
        //--------------------------------------------------
        // Following the Python snippet exactly:
        //   f1 = 2*(q1*q3 - q0*q2) - ax
        //   ...
        //   f4, f5, f6 for the magnetometer
        Eigen::Matrix<float, 6, 1> f;
        f(0) = 2.f * (q1 * q3 - q0 * q2) - a.x();
        f(1) = 2.f * (q0 * q1 + q2 * q3) - a.y();
        f(2) = 2.f * (0.5f - q1 * q1 - q2 * q2) - a.z();
        f(3) = 2.f * bx * (0.5f - q2 * q2 - q3 * q3) + 2.f * bz * (q1 * q3 - q0 * q2) - m.x();
        f(4) = 2.f * bx * (q1 * q2 - q0 * q3) + 2.f * bz * (q0 * q1 + q2 * q3) - m.y();
        f(5) = 2.f * bx * (q0 * q2 + q1 * q3) + 2.f * bz * (0.5f - q1 * q1 - q2 * q2) - m.z();

        //-------------------------------------
        // 4. Compute the Jacobian J (6x4 matrix)
        //-------------------------------------
        // Carefully transcribed from the Python snippet:
        Eigen::Matrix<float, 6, 4> J;
        // Row 0
        J(0, 0) = -2.f * q2;
        J(0, 1) = 2.f * q3;
        J(0, 2) = -2.f * q0;
        J(0, 3) = 2.f * q1;
        // Row 1
        J(1, 0) = 2.f * q1;
        J(1, 1) = 2.f * q0;
        J(1, 2) = 2.f * q3;
        J(1, 3) = 2.f * q2;
        // Row 2
        J(2, 0) = 0.f;
        J(2, 1) = -4.f * q1;
        J(2, 2) = -4.f * q2;
        J(2, 3) = 0.f;
        // Row 3
        J(3, 0) = -2.f * bz * q2;
        J(3, 1) = 2.f * bz * q3;
        J(3, 2) = -4.f * bx * q2 - 2.f * bz * q0;
        J(3, 3) = -4.f * bx * q3 + 2.f * bz * q1;
        // Row 4
        J(4, 0) = -2.f * bx * q3 + 2.f * bz * q1;
        J(4, 1) = 2.f * bx * q2 + 2.f * bz * q0;
        J(4, 2) = 2.f * bx * q1 + 2.f * bz * q3;
        J(4, 3) = -2.f * bx * q0 + 2.f * bz * q2;
        // Row 5
        J(5, 0) = 2.f * bx * q2;
        J(5, 1) = 2.f * bx * q3 - 4.f * bz * q1;
        J(5, 2) = 2.f * bx * q0 - 4.f * bz * q2;
        J(5, 3) = 2.f * bx * q1;

        //-----------------------------------
        // 5. Compute step = J^T * f (4x1)
        //-----------------------------------
        Eigen::Matrix<float, 4, 1> step = J.transpose() * f;

        // Normalize step
        float stepNorm = step.norm();
        if (stepNorm > 1e-12f)
        {
            step /= stepNorm;
        }

        //------------------------------------------------------------
        // 6. Gyroscope drift compensation
        //------------------------------------------------------------
        //   gyroscopeQuat = (0, gx, gy, gz)
        //   stepQuat      = (step[0], step[1], step[2], step[3])
        //   gyroscopeQuat -= (q.conjugate() * stepQuat) * 2 * samplePeriod * zeta
        Eigen::Quaternionf gyroscopeQuat(0.f, g.x(), g.y(), g.z());
        Eigen::Quaternionf stepQuat(step(0), step(1), step(2), step(3));

        Eigen::Quaternionf tmp = q.conjugate() * stepQuat;
        tmp.w() *= (2.f * samplePeriod * zeta);
        tmp.x() *= (2.f * samplePeriod * zeta);
        tmp.y() *= (2.f * samplePeriod * zeta);
        tmp.z() *= (2.f * samplePeriod * zeta);

        // Subtract from gyroscopeQuat
        gyroscopeQuat.w() -= tmp.w();
        gyroscopeQuat.x() -= tmp.x();
        gyroscopeQuat.y() -= tmp.y();
        gyroscopeQuat.z() -= tmp.z();

        //---------------------------------------------------
        // 7. Compute rate of change of quaternion (qDot)
        //---------------------------------------------------
        //   qDot = 0.5f * (q * gyroscopeQuat) - beta * stepQuat
        // We’ll do this by breaking it into steps:
        // (A) halfOmega = 0.5 * (q * gyroscopeQuat)
        Eigen::Quaternionf halfOmega = q * gyroscopeQuat;
        halfOmega.w() *= 0.5f;
        halfOmega.x() *= 0.5f;
        halfOmega.y() *= 0.5f;
        halfOmega.z() *= 0.5f;

        // (B) minusBetaStep = beta * stepQuat
        Eigen::Quaternionf minusBetaStep(beta * stepQuat.w(), beta * stepQuat.x(), beta * stepQuat.y(),
                                         beta * stepQuat.z());

        // (C) qDot = halfOmega - minusBetaStep
        Eigen::Quaternionf qDot(halfOmega.w() - minusBetaStep.w(), halfOmega.x() - minusBetaStep.x(),
                                halfOmega.y() - minusBetaStep.y(), halfOmega.z() - minusBetaStep.z());

        //--------------------------------------
        // 8. Integrate to yield new quaternion
        //--------------------------------------
        //   q += qDot * samplePeriod
        Eigen::Vector4f qVec(q.w(), q.x(), q.y(), q.z());
        Eigen::Vector4f qDotVec(qDot.w(), qDot.x(), qDot.y(), qDot.z());

        qVec += qDotVec * samplePeriod;
        // convert back to quaternion
        q = Eigen::Quaternionf(qVec[0], qVec[1], qVec[2], qVec[3]);

        // Finally, normalize the updated quaternion
        q.normalize();
    }
};