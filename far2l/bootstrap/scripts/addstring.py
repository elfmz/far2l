#!/usr/bin/env python3

import re
import sys

def add_translation(filepath):

    """
    Adds a new translation entry to farlang.templ.m4

    """

    # Get the new message ID from the user
    message_id = input("ID: ").strip()
    if not message_id:
        print("Message ID cannot be empty. Aborting.")
        return

    # Get the translations from the user
    ru_text = input("Русский перевод: ").strip()
    en_text = input("English translation: ").strip()

    if not ru_text or not en_text:
        print("Translations cannot be empty. Aborting.")
        return

    # Prepare the new entry
    new_entry = f"""{message_id}
"{ru_text}"
"{en_text}"
upd:"{en_text}"
upd:"{en_text}"
upd:"{en_text}"
upd:"{en_text}"
upd:"{en_text}"
upd:"{ru_text}"
upd:"{ru_text}"

"""

    try:
        with open(filepath, "r+") as f:
            lines = f.readlines()
            # Find the insertion point (line before "#Must be the last")
            insertion_point = -1
            for i, line in enumerate(lines):
                if line.strip() == "#Must be the last":
                    insertion_point = i
                    break

            if insertion_point == -1:
                print("Error: Could not find the insertion point ('#Must be the last') in the file.")
                return

            # Insert the new entry
            lines.insert(insertion_point, new_entry)

            # Write the modified file
            f.seek(0)
            f.writelines(lines)

        print(f"Translation added successfully to '{filepath}'.")

    except FileNotFoundError:
        print(f"Error: File '{filepath}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":

    filepath = "farlang.templ.m4"

    add_translation(filepath)
