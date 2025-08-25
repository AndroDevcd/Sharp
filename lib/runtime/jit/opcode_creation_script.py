# Dictionary mapping instruction names to their function parameters
instructions = {
    "nop": "",
    "int": "int flag",
    "movi": "int value, int outRegister",
    "ret": "int errState",
    "hlt": "",
    "newarray": "int inRegister, int ntype",
    "cast": "int classAddress",
    "mov8": "int outRegister, int registerToCast",
    "mov16": "int outRegister, int registerToCast",
    "mov32": "int outRegister, int registerToCast",
    "mov64": "int outRegister, int registerToCast",
    "movu8": "int outRegister, int registerToCast",
    "movu16": "int outRegister, int registerToCast",
    "movu32": "int outRegister, int registerToCast",
    "movu64": "int outRegister, int registerToCast",
    "rstore": "int registerToCast",
    "add": "int outRegister, int leftRegister, int rightRegister",
    "sub": "int outRegister, int leftRegister, int rightRegister",
    "mul": "int outRegister, int leftRegister, int rightRegister",
    "div": "int outRegister, int leftRegister, int rightRegister",
    "mod": "int outRegister, int leftRegister, int rightRegister",
    "iadd": "int outRegister, int value",
    "isub": "int outRegister, int value",
    "imul": "int outRegister, int value",
    "idiv": "int outRegister, int value",
    "imod": "int outRegister, int value",
    "pop": "",
    "inc": "int outRegister",
    "dec": "int outRegister",
    "movr": "int outRegister, int inRegister",
    "brh": "",
    "ife": "",
    "ifne": "",
    "lt": "int leftRegister, int rightRegister",
    "gt": "int leftRegister, int rightRegister",
    "lte": "int leftRegister, int rightRegister",
    "gte": "int leftRegister, int rightRegister",
    "movl": "int relFrameAddress",
    "movsl": "int relStackAddress",
    "sizeof": "int outRegister",
    "put": "int inRegister",
    "putc": "int inRegister",
    "checklen": "int inRegister",
    "jmp": "int address",
    "loadpc": "int outRegister",
    "pushobj": "",
    "del": "",
    "call": "int address",
    "newclass": "int address",
    "movn": "int address",
    "sleep": "int inRegister",
    "test": "int leftRegister, int rightRegister",
    "tne": "int leftRegister, int rightRegister",
    "lock": "",
    "ulock": "",
    "movg": "int address",
    "movnd": "int inRegister",
    "newobjarray": "int inRegister",
    "not": "int outRegister, int inRegister",
    "skip": "int instructionsToSkip",
    "loadval": "int outRegister",
    "shl": "int outRegister, int leftRegister, int rightRegister",
    "shr": "int outRegister, int leftRegister, int rightRegister",
    "skpe": "int inRegister, int address",
    "skne": "int inRegister, int address",
    "and": "int leftRegister, int rightRegister",
    "uand": "int leftRegister, int rightRegister",
    "or": "int leftRegister, int rightRegister",
    "xor": "int leftRegister, int rightRegister",
    "throw": "",
    "checknull": "int outRegister",
    "returnobj": "",
    "newclassarray": "int inRegister, int classAddress",
    "newstring": "int address",
    "addl": "int inRegister, int relFrameAddress",
    "subl": "int inRegister, int relFrameAddress",
    "mull": "int inRegister, int relFrameAddress",
    "divl": "int inRegister, int relFrameAddress",
    "modl": "int inRegister, int relFrameAddress",
    "iaddl": "int value, int relFrameAddress",
    "isubl": "int value, int relFrameAddress",
    "imull": "int value, int relFrameAddress",
    "idivl": "int value, int relFrameAddress",
    "imodl": "int value, int relFrameAddress",
    "loadl": "int outRegister, int relFrameAddress",
    "iaload": "int outRegister, int indexRegister",
    "popobj": "",
    "smovr": "int inRegister, int relStackAddress",
    "andl": "int inRegister, int relStackAddress",
    "orl": "int inRegister, int relStackAddress",
    "xorl": "int inRegister, int relStackAddress",
    "rmov": "int indexRegister, int inRegister",
    "smov": "int outRegister, int relStackAddress",
    "returnval": "int inRegister",
    "istore": "int value",
    "smovr_2": "int inRegister, int relFrameAddress",
    "istorel": "int relFrameAddress, int integerValue",
    "popl": "int relFrameAddress",
    "pushnull": "",
    "ipushl": "int relFrameAddress",
    "pushl": "int relFrameAddress",
    "itest": "int outRegister",
    "invoke_delegate": "int address, int argCount, bool isFunctionStatic",
    "get": "int outRegister",
    "isadd": "int relStackAddress, int value",
    "je": "int address",
    "jne": "int address",
    "ipopl": "int relFrameAddress",
    "cmp": "int inRegister, int value",
    "calld": "int inRegister",
    "varcast": "int varType, bool isArray",
    "tls_movl": "int address",
    "dup": "",
    "popobj_2": "",
    "swap": "",
    "ldc": "int outRegister, int address",
    "smovr_3": "int relFrameAddress",
    "neg": "int outRegister, int inRegister",
    "exp": "int outRegister, int leftRegister, int rightRegister",
    "is": "int outRegister, int type",
    "mov_abs": "int address",  # Note: This maps to emit_movabs in the virtual methods
    "load_abs": "int address", # Note: This maps to emit_loadabs in the virtual methods
    "iload": "int outRegister",
    "smovr_4": "int relFrameAddress1, int relFrameAddress2",
    "imov": "int inRegister"
}

import os

def create_instruction_files(folder_path):
    """
    Create C++ files for each instruction in the list.
    Files are named as op_{instruction}.cpp and contain boilerplate code.

    Args:
        folder_path (str): The directory path where files should be created/checked
    """
    # Ensure the folder exists
    if not os.path.exists(folder_path):
        print(f"Creating directory: {folder_path}")
        os.makedirs(folder_path)

    created_files = []
    skipped_files = []

def create_instruction_files(folder_path):
    """
    Create C++ files for each instruction in the dictionary.
    Files are named as op_{instruction}.cpp and contain boilerplate code with correct parameters.

    Args:
        folder_path (str): The directory path where files should be created/checked
    """
    # Ensure the folder exists
    if not os.path.exists(folder_path):
        print(f"Creating directory: {folder_path}")
        os.makedirs(folder_path)

    created_files = []
    skipped_files = []

    for instruction, parameters in instructions.items():
        filename = f"op_{instruction}.cpp"
        full_path = os.path.join(folder_path, filename)

        # Check if file already exists
        if os.path.exists(full_path):
            print(f"Skipping {filename} - file already exists")
            skipped_files.append(filename)
            continue

        # Create the file content
        function_name = f"emit_{instruction}"
        todo_comment = f"// TODO: Implement {instruction.upper()} opcode"

        # Handle special cases where instruction name differs from function name
        if instruction == "mov_abs":
            function_name = "emit_movabs"
        elif instruction == "load_abs":
            function_name = "emit_loadabs"

        file_content = f"""#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::{function_name}({parameters}) {{
    {todo_comment}
    return false;
}}
"""

        # Write the file
        try:
            with open(full_path, 'w') as f:
                f.write(file_content)
            print(f"Created {filename}")
            created_files.append(filename)
        except Exception as e:
            print(f"Error creating {filename}: {e}")

    # Summary
    print(f"\nSummary:")
    print(f"Created {len(created_files)} new files")
    print(f"Skipped {len(skipped_files)} existing files")

    return created_files, skipped_files

    # Summary
    print(f"\nSummary:")
    print(f"Created {len(created_files)} new files")
    print(f"Skipped {len(skipped_files)} existing files")

    return created_files, skipped_files

# Example usage
if __name__ == "__main__":
    # Specify the folder where you want the files created
    target_folder = "./arm/opcode"  # Change this to your desired path
    # target_folder = "/path/to/your/jit/opcodes"  # Example absolute path

    created, skipped = create_instruction_files(target_folder)