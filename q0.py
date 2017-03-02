import csv

with open('poi.tsv') as tsvin, open('poi_transformed.csv', 'w') as csvout:
    tsvin = csv.reader(tsvin, delimiter = '\t')
    csvout = csv.writer(csvout)

    for row in tsvin:
        new_line = []
        new_line.append(row[0])
        new_line.append(((float(row[2]) - 48.06000) - 5/111191)/(48.24900 - 48.06000) * 1000)
        new_line.append(((float(row[2]) - 48.06000) + 5/111191)/(48.24900 - 48.06000) * 1000)
        new_line.append(((float(row[3]) - 11.35800) - 5/74538)/(11.72400 - 11.25800) * 1000)
        new_line.append(((float(row[3]) - 11.35800) + 5/74538)/(11.72400 - 11.25800) * 1000)
        csvout.writerow(new_line)
