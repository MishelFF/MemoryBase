<?php

require "config.php";

header("Content-Type: application/json");


$media=$_GET["media"];


$stmt=$db->prepare("SELECT DISTINCT path FROM photobase.photo_images WHERE media_name=:media ORDER BY path");

$stmt->execute([":media"=>$media]);

$result=[];


while($row=$stmt->fetch(PDO::FETCH_ASSOC))
{
    $result[]=$row["path"];
}

echo json_encode($result);