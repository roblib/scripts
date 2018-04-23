import csv

def convert_dict_to_csv(toCSV, outputfilename):
    keys = list(toCSV[0].keys())
    with open(outputfilename, 'w', newline='') as output_file:
        dict_writer = csv.DictWriter(output_file, fieldnames=keys)
        dict_writer.writeheader()
        dict_writer.writerows(toCSV)
