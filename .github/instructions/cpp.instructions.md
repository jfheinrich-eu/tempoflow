---
applyTo: "**/*.{c,cc,cpp,cxx,h,hh,hpp,hxx}"
---

# C++ and VST3 Instructions

- Use C++17 and APIs supported by JUCE 9.0.2.
- Apply RAII, const-correctness, explicit ownership, and the C++ Core Guidelines.
- Keep real-time code allocation-free, lock-free, exception-free, and free from I/O or logging.
- Separate domain model, preset parsing, scheduling, DSP, processor integration, and UI.
- Never call UI or message-thread APIs from the audio callback.
- Keep parameter identifiers stable, explicit, and independent of display labels.
- Test block edges, transport discontinuities, tempo or meter changes, and extreme supported buffer sizes.
- Prefer deterministic pure functions for timing and semantic validation.
