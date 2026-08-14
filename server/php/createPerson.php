<?php

require "config.php";
header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
$displayName = trim($input["display_name"] ?? "");

if ($displayName === "") {
    http_response_code(400);
    echo json_encode(["error" => "display_name не может быть пустым"]);
    exit;
}

try {
    $stmt = $db->prepare("INSERT INTO photobase.people (display_name) VALUES (:display_name) RETURNING id");
    $stmt->execute([":display_name" => $displayName]);
    $id = (int)$stmt->fetchColumn();

    echo json_encode(["id" => $id,"display_name" => $displayName,"has_reference" => false,]);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}