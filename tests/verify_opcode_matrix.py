#!/usr/bin/env python3
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLI_PATH = ROOT / "tests" / "assembler_cli"


def run(cmd):
    result = subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(f"Command failed: {' '.join(cmd)}\nSTDOUT:\n{result.stdout}\nSTDERR:\n{result.stderr}")
    return result


def build_cli():
    cmd = [
        "g++",
        "-std=c++17",
        "-I.",
        "tests/assembler_cli.cpp",
        "app/src/main/cpp/assembler.cpp",
        "app/src/main/cpp/utils.cpp",
        "-o",
        str(CLI_PATH),
    ]
    run(cmd)


def parse_intel_hex(hex_text: str) -> dict[int, int]:
    image: dict[int, int] = {}
    for line in hex_text.strip().splitlines():
        line = line.strip()
        if not line:
            continue
        if not line.startswith(":"):
            raise ValueError(f"Invalid HEX line (missing colon): {line}")

        count = int(line[1:3], 16)
        addr = int(line[3:7], 16)
        rectype = int(line[7:9], 16)
        data = line[9 : 9 + count * 2]
        checksum = int(line[9 + count * 2 : 11 + count * 2], 16)

        total = count + (addr >> 8) + (addr & 0xFF) + rectype
        total += sum(int(data[i : i + 2], 16) for i in range(0, len(data), 2))
        total += checksum
        if (total & 0xFF) != 0:
            raise ValueError(f"Checksum mismatch in line: {line}")

        if rectype == 0x00:
            for i in range(count):
                image[addr + i] = int(data[i * 2 : i * 2 + 2], 16)
        elif rectype == 0x01:
            break
        else:
            raise ValueError(f"Unsupported HEX record type {rectype:02X}")
    return image


def assemble_source(source: str, include_defs: bool = False) -> dict[int, int]:
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        src_file = tmp / "test.asm"
        src_file.write_text(source, encoding="utf-8")

        cmd = [str(CLI_PATH), str(src_file)]
        if include_defs:
            cmd += ["Megaprocessor_defs.asm", "app/src/main/assets/Megaprocessor_defs.asm"]

        result = run(cmd)
        return parse_intel_hex(result.stdout)


def assert_bytes(test_name: str, source: str, expected: list[int]):
    image = assemble_source(source)
    actual = [image.get(i) for i in range(len(expected))]
    if actual != expected:
        raise AssertionError(
            f"{test_name} failed\nExpected: {[f'0x{x:02X}' for x in expected]}\n"
            f"Actual:   {[f'0x{x:02X}' if x is not None else None for x in actual]}\n"
            f"Source:\n{source}"
        )


def run_opcode_matrix():
    cases = {
        "NOP": ("NOP\n", [0xFF]),
        "RET": ("RET\n", [0xC6]),
        "RETI": ("RETI\n", [0xC7]),
        "MOVE R1,R0": ("MOVE R1,R0\n", [0x01]),
        "MOVE SP,R0": ("MOVE SP,R0\n", [0xF1]),
        "MOVE R0,SP": ("MOVE R0,SP\n", [0xF0]),
        "AND R0,R1": ("AND R0,R1\n", [0x14]),
        "XOR R0,R1": ("XOR R0,R1\n", [0x24]),
        "OR R0,R1": ("OR R0,R1\n", [0x34]),
        "ADD R0,R1": ("ADD R0,R1\n", [0x44]),
        "SUB R0,R1": ("SUB R0,R1\n", [0x64]),
        "CMP R0,R1": ("CMP R0,R1\n", [0x74]),
        "PUSH R3": ("PUSH R3\n", [0xCB]),
        "POP R3": ("POP R3\n", [0xC3]),
        "INC R2": ("INC R2\n", [0x58]),
        "DEC R2": ("DEC R2\n", [0x60]),
        "ADDQ R2,#-2": ("ADDQ R2,#-2\n", [0x64]),
        "TEST R3": ("TEST R3\n", [0x7F]),
        "LD.W R0,#0x1234": ("LD.W R0,#0x1234\n", [0xD0, 0x34, 0x12]),
        "LD.B R1,#0xAB": ("LD.B R1,#0xAB\n", [0xD5, 0xAB]),
        "ST.W 0x4567,R1": ("ST.W 0x4567,R1\n", [0xB9, 0x67, 0x45]),
        "LD.B (R2++),R0 pattern": ("LD.B R0,(R2++)\n", [0x94]),
        "BUC opcode": ("START:\nBUC START\n", [0xE0, 0xFE]),
        "BNE opcode": ("START:\nBNE START\n", [0xE6, 0xFE]),
        "BLE opcode": ("START:\nBLE START\n", [0xEF, 0xFE]),
        "BHS alias": ("START:\nBHS START\n", [0xE4, 0xFE]),
        "BLO alias": ("START:\nBLO START\n", [0xE5, 0xFE]),
        "JMP abs": ("JMP 0x1234\n", [0xF3, 0x34, 0x12]),
        "JSR abs": ("JSR 0x4567\n", [0xCF, 0x67, 0x45]),
        "ANDI": ("ANDI\n", [0xF4]),
        "ADDI": ("ADDI\n", [0xF6]),
        "NEGX": ("NEGX\n", [0xFE]),
    }

    for name, (source, expected) in cases.items():
        assert_bytes(name, source, expected)


def run_example_regression():
    source = (ROOT / "app/src/main/assets/example.asm").read_text(encoding="utf-8")
    expected_hex = (ROOT / "app/src/main/assets/example.hex").read_text(encoding="utf-8")

    expected_image = parse_intel_hex(expected_hex)
    actual_image = assemble_source(source, include_defs=True)

    if actual_image != expected_image:
        all_addresses = sorted(set(expected_image) | set(actual_image))
        mismatch_addr = next(
            addr for addr in all_addresses if expected_image.get(addr) != actual_image.get(addr)
        )
        raise AssertionError(
            "example.hex regression failed at address "
            f"0x{mismatch_addr:04X}: expected 0x{expected_image.get(mismatch_addr, -1):02X}, "
            f"got 0x{actual_image.get(mismatch_addr, -1):02X}"
        )


def main():
    build_cli()
    run_opcode_matrix()
    run_example_regression()
    print("All opcode matrix and example.hex regression checks passed.")


if __name__ == "__main__":
    main()
