#pragma once
#include "Color.hpp"
#include "Easing.hpp"
#include <functional>
#include <algorithm>

namespace cui {

// Smoothly chases a target value over time. Call SetTarget() whenever the
// goal changes (hover starts, a toggle flips, a tab gets picked) and
// Update(dt) once per frame to step it forward.
class AnimatedFloat {
public:
    explicit AnimatedFloat(float initial = 0.0f, float durationSeconds = 0.15f,
                            float (*ease)(float) = easing::CubicOut)
        : from_(initial), to_(initial), current_(initial),
          duration_(durationSeconds), ease_(ease) {}

    void SetDuration(float seconds) { duration_ = seconds; }
    void SetEasing(float (*ease)(float)) { ease_ = ease; }

    void SetTarget(float target, bool snap = false) {
        if (snap) {
            from_ = to_ = current_ = target;
            elapsed_ = duration_;
            return;
        }
        if (target == to_) return;
        from_ = current_;
        to_ = target;
        elapsed_ = 0.0f;
    }

    // Advance the animation by dt seconds. Returns the updated value.
    float Update(float dt) {
        if (elapsed_ < duration_) {
            elapsed_ = std::min(elapsed_ + dt, duration_);
            float t = duration_ <= 0.0f ? 1.0f : elapsed_ / duration_;
            current_ = from_ + (to_ - from_) * ease_(t);
        } else {
            current_ = to_;
        }
        return current_;
    }

    float Value() const { return current_; }
    float Target() const { return to_; }
    bool IsAnimating() const { return elapsed_ < duration_; }

private:
    float from_, to_, current_;
    float elapsed_ = 0.0f;
    float duration_;
    float (*ease_)(float);
};

// Same idea, but for colors. Drives things like a checkbox tinting from
// grey to accent color on enable.
class AnimatedColor {
public:
    explicit AnimatedColor(Color initial = Color(255, 255, 255, 255),
                            float durationSeconds = 0.2f,
                            float (*ease)(float) = easing::CubicOut)
        : from_(initial), to_(initial), current_(initial),
          duration_(durationSeconds), ease_(ease) {}

    void SetTarget(Color target, bool snap = false) {
        if (snap) {
            from_ = to_ = current_ = target;
            elapsed_ = duration_;
            return;
        }
        from_ = current_;
        to_ = target;
        elapsed_ = 0.0f;
    }

    Color Update(float dt) {
        if (elapsed_ < duration_) {
            elapsed_ = std::min(elapsed_ + dt, duration_);
            float t = duration_ <= 0.0f ? 1.0f : elapsed_ / duration_;
            current_ = Color::Lerp(from_, to_, ease_(t));
        } else {
            current_ = to_;
        }
        return current_;
    }

    Color Value() const { return current_; }

private:
    Color from_, to_, current_;
    float elapsed_ = 0.0f;
    float duration_;
    float (*ease_)(float);
};

// Cycles hue over time, for an optional rainbow accent mode.
class HueCycler {
public:
    explicit HueCycler(float degreesPerSecond = 60.0f) : speed_(degreesPerSecond) {}
    Color Update(float dt, float saturation = 0.65f, float value = 0.95f) {
        hue_ = std::fmod(hue_ + speed_ * dt, 360.0f);
        if (hue_ < 0) hue_ += 360.0f;
        return Color::FromHSV(hue_, saturation, value);
    }
    void SetSpeed(float degreesPerSecond) { speed_ = degreesPerSecond; }

private:
    float hue_ = 0.0f;
    float speed_;
};

// A gentle, endless sine wave in [0,1] - not chasing a target like
// AnimatedFloat, just breathing. Used for the soft glow pulse on active
// toggles and for slowly shimmering an accent gradient.
class Oscillator {
public:
    explicit Oscillator(float cyclesPerSecond = 0.6f) : speed_(cyclesPerSecond) {}
    float Update(float dt) {
        phase_ += speed_ * dt * 6.2831853f; // 2*pi per full cycle
        value_ = std::sin(phase_) * 0.5f + 0.5f;
        return value_;
    }
    float Value() const { return value_; }

private:
    float phase_ = 0.0f;
    float speed_;
    float value_ = 0.5f;
};

} // namespace cui
