import argparse
import wave
from pathlib import Path

import numpy as np
from numpy.typing import NDArray  # type: ignore


def generate_square_wave(
    frequency: float, duration: float, volume: float, sample_rate: int
) -> NDArray[np.int16]:
    t: NDArray[np.float32] = np.linspace(
        0, duration, int(sample_rate * duration), endpoint=False, dtype=np.float32
    )
    waveform: NDArray[np.float32] = 0.5 * np.sign(np.sin(2 * np.pi * frequency * t))
    return (waveform * volume * 32767).astype(np.int16)


def write_wave_file(
    filename: Path, samples: NDArray[np.int16], sample_rate: int
) -> None:
    with wave.open(str(filename), "w") as wf:
        wf.setnchannels(1)  # mono
        wf.setsampwidth(2)  # 2 bytes per sample (16-bit)
        wf.setframerate(sample_rate)
        wf.writeframes(samples.tobytes())


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate a square wave .wav file.")
    parser.add_argument(
        "--frequency",
        type=float,
        default=440.0,
        help="Frequency of the tone in Hz (default: 440.0)",
    )
    parser.add_argument(
        "--duration", type=float, default=1.0, help="Duration in seconds (default: 1.0)"
    )
    parser.add_argument(
        "--volume", type=float, default=0.5, help="Volume (0.0 to 1.0, default: 0.5)"
    )
    parser.add_argument(
        "--samplerate",
        type=int,
        default=44100,
        help="Sample rate in Hz (default: 44100)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("chip8_beep.wav"),
        help="Output WAV file (default: chip8_beep.wav)",
    )

    args = parser.parse_args()

    samples: NDArray[np.int16] = generate_square_wave(
        args.frequency, args.duration, args.volume, args.samplerate
    )
    write_wave_file(args.output, samples, args.samplerate)

    print(f"Beep sound written to {args.output.resolve()}")


if __name__ == "__main__":
    main()
