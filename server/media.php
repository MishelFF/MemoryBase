<?php

require "config.php";

header("Content-Type: application/json");


$sql = "SELECT DISTINCT media_name FROM photobase.photo_images ORDER BY media_name";

$stmt = $db->query($sql);

$result = [];

while($row = $stmt->fetch(PDO::FETCH_ASSOC))
{
    $result[] = $row["media_name"];
}


echo json_encode($result);