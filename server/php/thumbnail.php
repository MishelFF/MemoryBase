<?php

require "config.php";

$id =  intval($_GET["id"]);


$stmt=$db->prepare("SELECT image_data FROM photobase.photo_thumbnails WHERE photo_id=:id");

$stmt->execute([ ":id"=>$id]);

$data=$stmt->fetchColumn();

header(
"Content-Type: image/jpeg"
);

if (is_resource($data)) {
    echo stream_get_contents($data);
} else {
    echo $data;
}
