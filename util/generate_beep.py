import argparse
import math
import wave
from array import array
from pathlib import Path

def generate_square_wave(
    frequency: float,
    duration: float,
    volume: float,
    sample_rate: int,
) -> array:
    samples = array("h")
    total_samples = int(sample_rate * duration)
    clamped_volume = max(0.0, min(1.0, volume))
    amplitude = int(clamped_volume * 32767)

    for i in range(total_samples):
        theta = 2.0 * math.pi * frequency * (i / sample_rate)
        sample = amplitude if math.sin(theta) >= 0.0 else -amplitude
        samples.append(sample)

    return samples


def write_wave_file(filename: Path, samples: array, sample_rate: int) -> None:
    filename.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(filename), "wb") as wf:
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

    samples = generate_square_wave(
        args.frequency, args.duration, args.volume, args.samplerate
    )
    write_wave_file(args.output, samples, args.samplerate)

    print(f"Beep sound written to {args.output.resolve()}")


if __name__ == "__main__":
    main()
