module;

#include <atomic>
#include <chrono>

export module opn.System.Service.Time;

import opn.System.ServiceInterface;
import opn.Utils.Exceptions;

export namespace opn {
    class Time final : public Service<Time> {
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::high_resolution_clock::time_point;
        using Duration = std::chrono::duration<double>;

        static std::atomic_bool s_initialized;

        mutable TimePoint m_startTime{};
        mutable TimePoint m_lastFrameTime{};

        mutable double m_deltaTimeReal{0.0};
        mutable double m_deltaTimeGame{0.0};
        mutable double m_timeScale{0.0};

        mutable bool m_paused{false};

        mutable double m_physicsRate{60.0};
        mutable double m_fixedDeltaTime{1.0 / 60.0};
        mutable double m_accumulator{0.0};

    protected:
        void onInit() override {
            m_startTime = Clock::now();
            m_lastFrameTime = m_startTime;
            m_accumulator = 0.0;
        }

        void onShutdown() override {
            m_deltaTimeReal = 0.0;
            m_deltaTimeGame = 0.0;
        }

    public:
        void update() const {
            const auto currentTime = Clock::now();
            m_deltaTimeReal = std::chrono::duration<double>(currentTime - m_lastFrameTime).count();
            m_lastFrameTime = currentTime;

            m_deltaTimeGame = m_paused ? 0.0 : m_deltaTimeReal * m_timeScale;

            m_accumulator += m_deltaTimeGame;
        }

        bool checkPhysicsStep() const {
            if (m_accumulator >= m_fixedDeltaTime) {
                m_accumulator -= m_fixedDeltaTime;
                return true;
            }
            return false;
        }

        [[nodiscard]] double getDeltaTime() const { return m_deltaTimeGame; }
        [[nodiscard]] double getRealDeltaTime() const { return m_deltaTimeReal; }
        [[nodiscard]] double getFixedDeltaTime() const { return m_fixedDeltaTime; }

        [[nodiscard]] double getPhysicsInterpolationFactor() const {
            return m_accumulator / m_fixedDeltaTime;
        }

        void setPaused(const bool _paused) const { m_paused = _paused; }
        void setTimeScale(const double _scale) const { m_timeScale = _scale; }

        void setPhysicsRate(const double _hz) const {
            m_physicsRate = _hz;
            m_fixedDeltaTime = 1.0 / _hz;
        }
    };
};
