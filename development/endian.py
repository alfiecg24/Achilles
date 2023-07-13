import sys

def swap_endianness(data):
    # Split the data into bytes
    bytes_data = [data[i:i+2] for i in range(0, len(data), 2)]
    
    # Reverse the order of each byte
    swapped_data = [byte[::-1] for byte in bytes_data]
    
    # Join the swapped bytes and return the result but not as a string
    return b''.join(swapped_data)

def main():
    # Check if the .bin file path is provided
    if len(sys.argv) < 2:
        print("Please provide the path to the .bin file as a command-line argument.")
        return

    # Read the file contents
    file_path = sys.argv[1]
    try:
        with open(file_path, 'rb') as file:
            data = file.read()
    except FileNotFoundError:
        print(f"File '{file_path}' not found.")
        return

    # Swap the endianness
    swapped_data = swap_endianness(data)
    
    # Write the swapped data to a new file
    output_path = f"swapped_{file_path}"
    with open(output_path, 'wb') as file:
        file.write(swapped_data)

    print(f"Endianness swapped file saved as '{output_path}'.")

if __name__ == "__main__":
    main()