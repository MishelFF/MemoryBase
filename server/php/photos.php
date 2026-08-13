<?php

require "config.php";

header("Content-Type: application/json");


$media=$_GET["media"];
$path=$_GET["path"];


$stmt=$db->prepare("SELECT id, file, width, height, date_creation,comment,device,maker,filesize,ext,path,media_name,md5sum,rotation,latitude,longitude,date_available  FROM photobase.photo_images WHERE media_name=:media AND path=:path ORDER BY file");

// Логируем входящие GET-параметры
#$timestamp = date("Y-m-d H:i:s");
#$logMessage = "[$timestamp] Входящий запрос: media='$media', path='$path'\n";

#$sqlText = "SELECT id, file, width, height, date_creation FROM photobase.photo_images WHERE media_name=:media AND path=:path ORDER BY file";
#$debugSql = str_replace(
#    [":media", ":path"], 
#    ["'" . addslashes($media) . "'", "'" . addslashes($path) . "'"], 
#    $sqlText
#);
#$logMessage .= "[$timestamp] Выполняемый SQL: $debugSql\n";

// Записываем параметры и SQL в файл api_debug.log (создастся в той же папке)
#file_put_contents("api_debug.log", $logMessage, FILE_APPEND);

$stmt->execute([":media"=>$media,":path"=>$path]);

$result=[];

while($row=$stmt->fetch(PDO::FETCH_ASSOC))
{
    $result[]=$row;
}

echo json_encode($result);