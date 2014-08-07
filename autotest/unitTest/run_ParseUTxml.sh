if [ $# -ne 1 ];then
    echo Please input $0 [report dir]
    exit 1
fi

REPORT=$1
if [ ! -e ${REPORT} ];then
    echo "The directory of ${REPORT} dose't not exit,please check the test log"
    exit 1
fi

UT_Failed_Num=0
parse_unittest() {
    res=$1
    echo ${res}
    echo Start to parse unittest results of $res
    if [ -e $res ];then
    tests=`cat $res | grep "<testsuites" | awk -F " " '{print $2;}' | awk -F "\"" '{print $2;}'`
    fails=`cat $res | grep "<testsuites" | awk -F " " '{print $3;}' | awk -F "\"" '{print $2;}'`
    times=`cat $res | grep "<testsuites" | awk -F " " '{print $6;}' | awk -F "\"" '{print $2;}'`
    waste=`cat $res | grep "<testsuites" | awk -F " " '{print $7;}' | awk -F "\"" '{print $2;}'`
    msg="Total testcases: $tests, failed: $fails,time:$waste seconds, at$times,xml:$res"
    echo ${msg}
    UT_Failed_Num=$((${UT_Failed_Num}+${fails}))
cat >> mail.log << EOF
<style>
.fail {
    background-color: yellow;
}
</style>
<br>
   <table  style="width:600px" cellspacing="0" border="1" width="100%">
    <thead>
    <tr>
        <td>Total unit test cases</td>
        <td>Failed</td>
        <td>Time</td>
        <td>Date</td>
    </tr>
    </thead>
       <tbody>
        <tr style="text-align:center; font-weight: bold;">
            <td>${tests}</td>
            <td><font class="fail">${fails}</font></td>
            <td>${waste}</td>
            <td>${times}</td>
        </tr>
    </tbody>
    </table>
<br>
EOF
   fi
}

xmlcount=`ls $REPORT | wc -l`
xmlfiles=`ls $REPORT`
if [ ${xmlcount} -eq 0 ];
then echo There is nothing xml files generated at $REPORT
    exit 1
fi
for file in $xmlfiles;do
   parse_unittest $REPORT/$file
done
if [ ${UT_Failed_Num} = "0" ];then
echo Total $xmlcount files at $REPORT,all sucessful
exit 0
else
echo Total $xmlcount files at $REPORT,${UT_Failed_Num} error cases
exit 2
fi
