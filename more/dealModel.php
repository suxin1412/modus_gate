<?php
function thisIsGET(){
    $serverHost = "192.168.1.106";
    $userName = "root";
    $password = "raspberry1412";
    $conn = mysqli_connect($serverHost, $userName, $password);
    if (!$conn){
        die("Fail:".mysqli_connect_error());
    }
    mysqli_query($conn , "set names utf8");
    mysqli_select_db( $conn, 'Gateway' );
    if ($_GET['cmd']=="current"){
        $logFill = fopen("./JSONconfig/DomeModel_1.json","r") or die("Unable to open file!");;
        $logText = fread($logFill,filesize("./JSONconfig/DomeModel_1.json"));
        fclose($logFill);
        echo $logText;
    }elseif ($_GET['cmd']=="modellist"){
        $fileList = array();
        $i = 0;
        $sqlCMD = 'SELECT * FROM configFiles';
        $retval = mysqli_query( $conn, $sqlCMD );//执行查询语句
        while($row = mysqli_fetch_array($retval, MYSQLI_ASSOC))//读出每一行数据
        {
            $fileList[$i]=$row;
            $i = $i + 1;
        }
        echo json_encode($fileList);
    }elseif ($_GET['cmd']=="requestmodel"){
        $sqlCMD = 'SELECT path FROM configFiles WHERE fileName=\''.$_GET["name"]."';";
        $retval = mysqli_query( $conn, $sqlCMD );//执行查询语句
        while($row = mysqli_fetch_array($retval, MYSQLI_ASSOC))//读出每一行数据
        {
            $logFill = fopen($row['path'],"r") or die("Unable to open file!");;
            $logText = fread($logFill,filesize($row['path']));
            fclose($logFill);
            echo $logText;
        }
    }elseif ($_GET['cmd']=="createNewConfigFile"){
        $newFileName = 'Model'.time().'_'.mt_rand(10,100);
        $sqlCMD = "INSERT INTO configFiles  ( fileName, path, date, isUsed, currentShow)".
                " VALUES ( '{$newFileName}', '/var/www/html/Gateway/more/JSONconfig/{$newFileName}.json', NOW(), 0, 0);";
        $retval = mysqli_query( $conn, $sqlCMD );//执行查询语句
        echo ">".$retval;
        $myfile = fopen("/var/www/html/Gateway/more/JSONconfig/{$newFileName}.json", "w");
        fclose($myfile);
        $tempData = date("Y-m-d");
        file_put_contents("/var/www/html/Gateway/more/JSONconfig/{$newFileName}.json","{\"Name\": \"{$newFileName}\",\"Date\":\"{$tempData}\",\"AddressSpace\":[]}");
    }elseif ($_GET['cmd']=="deleteConfigFile"){
        $sqlCMD = 'SELECT path FROM configFiles WHERE fileName=\''.$_GET["name"]."';";
        $retval = mysqli_query( $conn, $sqlCMD );//执行查询语句
        while($row = mysqli_fetch_array($retval, MYSQLI_ASSOC))//读出每一行数据
        {
            if (!unlink($row['path'])){
                echo "err";
                mysqli_close($conn);
                return ;
            }
        }
        $sqlCMD = 'DELETE FROM configFiles WHERE fileName=\''.$_GET["name"]."';";
        $retval = mysqli_query( $conn, $sqlCMD );//执行查询语句
    }elseif ($_GET['cmd']=="useCurrentConfigFile"){
        $sqlCMD = 'UPDATE configFiles SET isUsed=0 ;';
        $retval = mysqli_query( $conn, $sqlCMD );//执行查询语句
        $sqlCMD = 'UPDATE configFiles SET isUsed=1 WHERE fileName=\''.$_GET["name"]."';";
        $retval = mysqli_query( $conn, $sqlCMD );//执行查询语句
    }
    mysqli_close($conn);
}
function thisIsPOST(){
    $content = json_decode(file_get_contents('php://input'),true);
    $serverHost = "192.168.1.106";
    $userName = "root";
    $password = "raspberry1412";
    $conn = mysqli_connect($serverHost, $userName, $password);
    if (!$conn){
        die("Fail:".mysqli_connect_error());
    }
    mysqli_query($conn , "set names utf8");
    mysqli_select_db( $conn, 'Gateway' );
    $sqlCMD = 'SELECT path FROM configFiles WHERE fileName=\''.$content['OldName']."';";
    $retval = mysqli_query( $conn, $sqlCMD );//执行查询语句
    while($row = mysqli_fetch_array($retval, MYSQLI_ASSOC))//读出每一行数据
    {
        echo file_put_contents($row['path'],file_get_contents('php://input'));
    }
    $sqlCMD = 'UPDATE configFiles SET fileName=\''.$content['Name'].'\'  WHERE fileName=\''.$content['OldName']."';";
    $retval = mysqli_query( $conn, $sqlCMD );//执行查询语句
    echo ($retval==true)?'true':'false';
    echo $sqlCMD;
    mysqli_close($conn);
}

if ($_SERVER['REQUEST_METHOD']=='GET'){
    thisIsGET();
}else{
    thisIsPOST();
}




?>