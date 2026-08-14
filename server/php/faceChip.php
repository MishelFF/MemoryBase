<?php

require "config.php";

// kind: "region" | "person"
$kind = $_GET["kind"] ?? "";
$id   = $_GET["id"] ?? "";

if (!ctype_digit((string)$id)) {
    http_response_code(400);
    header("Content-Type: application/json");
    echo json_encode(["error" => "Некорректный id"]);
    exit;
}

if ($kind === "region") {
    $sql = "SELECT face_chip AS chip FROM photobase.photo_regions WHERE id = :id";
} elseif ($kind === "person") {
    $sql = "SELECT reference_chip AS chip FROM photobase.people WHERE id = :id";
} else {
    http_response_code(400);
    header("Content-Type: application/json");
    echo json_encode(["error" => "Неизвестный тип: {$kind}"]);
    exit;
}

$stmt = $db->prepare($sql);
$stmt->execute([":id" => (int)$id]);
$row = $stmt->fetch(PDO::FETCH_ASSOC);

if (!$row) {
    http_response_code(404);
    header("Content-Type: application/json");
    echo json_encode(["error" => "Не найдено"]);
    exit;
}

$chip = $row["chip"];

if (is_resource($chip)) {
    $chip = stream_get_contents($chip);
}

if ($chip === null || $chip === "") {
    http_response_code(204);
    exit;
}

header("Content-Type: image/jpeg");
header("Content-Length: " . strlen($chip));
echo $chip;