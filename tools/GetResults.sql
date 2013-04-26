use libmatcher;

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 2
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "libz.so.1.2.3.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 3
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "modprobe.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 4
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "modinfo.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 5
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "depmod.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 6
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "gzip.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 7
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "libwvstream.so.4.6.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 8
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "libeshell.so.0.0.0.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 9
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "zlibmodule.so.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 10
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "zipcloak.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 11
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "zipsplit.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 12
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "gnome-panel.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 13
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "zipnote.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 14
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "zip.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 15
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "nautilus.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';

SELECT file1.file_id, file1.path, func1.func_id, func1.func_name, func1.cyccomp, func1.num_ngrams,
file2.file_id, file2.path, func2.func_id, func2.func_name, func2.cyccomp, func2.num_ngrams,
`dist_cosim`.cosim, `dist`.dist
FROM `dist`, `function` AS func1, `function` AS func2,
`file` AS file1, `file` AS file2, `dist_cosim`
WHERE file2.file_id = 16
AND dist.func1_id = func1.func_id
AND file1.file_id = func1.file_id
AND dist.func2_id = func2.func_id
AND file2.file_id = func2.file_id
AND dist_cosim.func1_id = func1.func_id
AND dist_cosim.func2_id = func2.func_id
INTO OUTFILE "rsync.csv"
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';
