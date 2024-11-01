const fs = require("fs");

function benchmarkJson() {
    const data = fs.readFileSync("E:\\code\\c\\json\\tests\\really-big-json-file.json", {encoding: "utf-8"});
    const start = usec_timestamp();
    JSON.parse(data);
    const end = usec_timestamp();
    
    console.log("Parsing time: %i microseconds", end - start);
}

function usec_timestamp() {
    const hrTime = process.hrtime();
    return hrTime[0] * 1000000 + hrTime[1] / 1000;
}

benchmarkJson();