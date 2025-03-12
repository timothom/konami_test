#!/usr/bin/python3
import sys
import xml.etree.ElementTree as ET

# XML data pattern
xml_data = {
    "root": "Message",
    "children": {
        "Command": "Print",
        "Data": {
            "Row": [
                {
                    "Description": "Name",
                    "Value": "Mr. Joe Chase"
                },
                {
                    "Description": "Address",
                    "Value": "123 Anywhere Lane"
                }
            ]
        }
    }
}

def create_xml(data, filename="output.xml"):
    """
    Generate XML files suitible for testing Konami server/client 
    """
    root = ET.Element(data["root"])
    
    def add_elements(parent, elements):
        for key, value in elements.items():
            if isinstance(value, dict):
                element = ET.SubElement(parent, key)
                add_elements(element, value)
            elif isinstance(value, list):
                 for item in value:
                    if isinstance(item, dict):
                        element = ET.SubElement(parent, key)
                        add_elements(element, item)
                    else:
                        element = ET.SubElement(parent, key)
                        element.text = str(value)
            else:
                element = ET.SubElement(parent, key)
                element.text = str(value)  
    
    add_elements(root, data.get("children", {}))
    
    tree = ET.ElementTree(root)
    ET.indent(tree, space="  ", level=0)
    tree.write(filename, encoding="UTF-8", xml_declaration=True)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        try:
            number_of_xmls = int(sys.argv[1])
            print("Generating ",number_of_xmls, " xml files in the current directory")
        except ValueError:
            print("Arguement passed in was not an integer.  Pass in the number of XML files to create")
            sys.exit(1)
    else:
        create_xml(xml_data, filename="xml-message.txt")
        sys.exit(0)

    for n in range(number_of_xmls):
        create_xml(xml_data, filename="xml-message" + str(n) + ".txt")






