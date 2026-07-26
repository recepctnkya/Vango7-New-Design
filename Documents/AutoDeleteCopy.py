import os
import shutil
import tkinter as tk
from tkinter import messagebox

# Define paths
path1 = r"C:\Users\nakau\Desktop\VanGo4.3\Vango_ESPIDF\components\ui_files"
path2 = r"C:\Users\nakau\Desktop\VanGo4.3\Vango_ESPIDF\Documents\SquareLine_Project\UI_Files"
path3 = r"C:\Users\nakau\Desktop\VanGo4.3\Vango_ESPIDF\main"
path_txt = r"C:\Users\nakau\Desktop\VanGo4.3\Vango_ESPIDF\ui_events.txt"
path_h_txt = r"C:\Users\nakau\Desktop\VanGo4.3\Vango_ESPIDF\ui_events.h.txt"
path_c = r"C:\Users\nakau\Desktop\VanGo4.3\Vango_ESPIDF\components\ui_files\ui_events.c"
path_h = r"C:\Users\nakau\Desktop\VanGo4.3\Vango_ESPIDF\components\ui_files\ui_events.h"

# Files to keep (do not copy)
files_to_keep = [
    "filelist.txt",
    "CMakeLists.txt"
]


# Function to update CMakeLists.txt by adding or removing entries
def update_cmakelists(new_file=None, remove_file=None):
    if new_file and not new_file.endswith(".c"):
        print(f"Skipped adding to CMakeLists.txt (not a .c file): {new_file}")
        return  # Skip if the file is not a .c file
    
    cmake_file = os.path.join(path3, "CMakeLists.txt")
    cmake_entry = f'    "../components/ui_files/{new_file.replace("\\", "/")}"\n' if new_file else None
    cmake_remove_entry = f'"../components/ui_files/{remove_file.replace("\\", "/")}"\n' if remove_file else None

    # Read current content of CMakeLists.txt
    with open(cmake_file, "r") as f:
        lines = f.readlines()
    
    # Remove the entry if requested
    if cmake_remove_entry:
        if cmake_remove_entry in lines:
            lines.remove(cmake_remove_entry)
            print(f"Removed from CMakeLists.txt: {cmake_remove_entry.strip()}")
    
    # Check if new entry already exists in the file
    if cmake_entry and cmake_entry not in lines:
        # Find the line containing "settings_page.c"
        insert_index = None
        for i, line in enumerate(lines):
            if "settings_page.c" in line:
                insert_index = i + 1  # Insert after the "settings_page.c" line
                break

        # If "settings_page.c" is found, insert the new file after it
        if insert_index is not None:
            lines.insert(insert_index, cmake_entry)
            print(f"Added to CMakeLists.txt: {cmake_entry.strip()}")
    
    # Write the updated content back to the CMakeLists.txt file
    with open(cmake_file, "w") as f:
        f.writelines(lines)


# Function to synchronize CMakeLists.txt with files in path1
def synchronize_cmakelists():
    excluded_files = [
        "main.c",
        "lvgl_demo_ui.c",
        "display_manager.c",
        "settings_page.c",
        "data_manager.c"
    ]

    cmake_file = os.path.join(path3, "CMakeLists.txt")

    # Get the list of .c files in path1
    path1_files = set(
        file for file in os.listdir(path1)
        if file.endswith(".c")
    )

    # Read current content of CMakeLists.txt
    with open(cmake_file, "r") as f:
        lines = f.readlines()

    # Identify lines to keep or remove
    updated_lines = []
    cmake_files = set()
    for line in lines:
        if line.strip().startswith('"../components/ui_files/') and line.strip().endswith('.c"'):
            file_name = line.split("/")[-1].strip('" \n')
            cmake_files.add(file_name)
            # If file is in path1 or is an excluded file, keep it
            if file_name in path1_files or file_name in excluded_files:
                updated_lines.append(line)
            else:
                print(f"Removed from CMakeLists.txt: {file_name}")
        else:
            updated_lines.append(line)

    # Add missing files from path1 to CMakeLists.txt
    missing_files = path1_files - cmake_files
    if missing_files:
        # Find the line containing "settings_page.c" to insert after it
        settings_page_index = None
        for i, line in enumerate(updated_lines):
            if "settings_page.c" in line:
                settings_page_index = i
                break
        
        # Insert missing files after settings_page.c
        if settings_page_index is not None:
            for file in sorted(missing_files):  # Optional: Sort files for consistent order
                new_entry = f'    "../components/ui_files/{file}"\n'
                updated_lines.insert(settings_page_index + 1, new_entry)
                settings_page_index += 1  # Update index for next insertion
                print(f"Added to CMakeLists.txt: {file}")
        else:
            # Fallback: insert at the end if settings_page.c not found
            for file in sorted(missing_files):
                new_entry = f'    "../components/ui_files/{file}"\n'
                updated_lines.insert(-1, new_entry)
                print(f"Added to CMakeLists.txt: {file}")

    # Write the updated content back to the CMakeLists.txt file
    with open(cmake_file, "w") as f:
        f.writelines(updated_lines)



def update_eventc_file():
    # Copy content from txt to c file
    try:
        with open(path_txt, 'r', encoding='utf-8') as src, open(path_c, 'w', encoding='utf-8') as dest:
            shutil.copyfileobj(src, dest)
        print("Content copied successfully.")
    except Exception as e:
        print(f"Error: {e}")
def update_eventh_file():
    # Copy content from txt to h file
    try:
        with open(path_h_txt, 'r', encoding='utf-8') as src, open(path_h, 'w', encoding='utf-8') as dest:
            shutil.copyfileobj(src, dest)
        print("Content copied successfully.")
    except Exception as e:
        print(f"Error: {e}")
# Function to execute the copy and update tasks
def execute_task():
    try:
        # Delete all .c and .h files in path1
        for root, dirs, files in os.walk(path1):
            for file in files:
                if file.endswith(".c") or file.endswith(".h"):
                    file_path = os.path.join(root, file)
                    os.remove(file_path)
                    print(f"Deleted: {file}")

        # Copy all .c and .h files from path2 to path1
        for root, dirs, files in os.walk(path2):
            for file in files:
                if file.endswith(".c") or file.endswith(".h"):
                    src_file = os.path.join(root, file)
                    dst_file = os.path.join(path1, file)
                    shutil.copy(src_file, dst_file)
                    print(f"Copied: {file}")

                    # Add the new file to CMakeLists.txt
                    update_cmakelists(new_file=file)
                    synchronize_cmakelists()
        update_eventc_file()
        update_eventh_file()


        # Show a success message
    

    except Exception as e:
        # Show error message
        messagebox.showerror("Error", f"An error occurred: {e}")

# Create the main window
root = tk.Tk()
root.title("File Reset and Update")

# Set window size
root.geometry("300x150")

# Create the button to trigger the process
button = tk.Button(root, text="Start Process", command=execute_task, height=2, width=20)
button.pack(pady=50)

# Start the GUI event loop
root.mainloop()