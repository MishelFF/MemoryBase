<?php

require "config.php";

header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
if (!is_array($input)) $input = [];

$media          = isset($input["media"]) && is_array($input["media"]) ? $input["media"] : [];
$mediaEnabled   = !empty($input["mediaEnabled"]);

$personIds      = isset($input["personIds"]) && is_array($input["personIds"]) ? array_map("intval", $input["personIds"]) : [];
$facesEnabled   = !empty($input["facesEnabled"]);
$facesUseDescriptor   = !empty($input["facesUseDescriptor"]);
$similarityThreshold  = isset($input["similarityThreshold"]) ? (float)$input["similarityThreshold"] : 0.0;

$dateEnabled    = !empty($input["dateEnabled"]);
$dateFrom       = $input["dateFrom"] ?? null;
$dateTo         = $input["dateTo"] ?? null;

$limitEnabled   = !empty($input["limitEnabled"]);
$maxCount       = isset($input["maxCount"]) ? (int)$input["maxCount"] : 0;

$sortBy         = $input["sortBy"] ?? "date";

$mediaPlaceholders = [];
foreach ($media as $i => $v) $mediaPlaceholders[] = ":media{$i}";

$personPlaceholders = [];
foreach ($personIds as $i => $v) $personPlaceholders[] = ":person{$i}";

$needsScores = $facesEnabled && !empty($personIds);

$scoresCte = "";
if ($needsScores) {
    $personList = implode(",", $personPlaceholders);

    if ($facesUseDescriptor) {
        $scoresCte = "
            WITH selected_people AS (
                SELECT id, reference_descriptor FROM photobase.people
                WHERE id IN ({$personList}) AND reference_descriptor IS NOT NULL
            ),
            matches AS (
                SELECT pr.photo_id, sp.id AS person_id,
                       MIN(pr.descriptor::photobase.vector <-> sp.reference_descriptor::photobase.vector) AS best_distance
                FROM photobase.photo_regions pr
                CROSS JOIN selected_people sp
                WHERE pr.descriptor IS NOT NULL
                GROUP BY pr.photo_id, sp.id
            ),
            photo_scores AS (
                SELECT photo_id, COUNT(*) AS match_count
                FROM matches
                WHERE best_distance < :threshold
                GROUP BY photo_id
            )
        ";
    } else {
        $scoresCte = "
            WITH photo_scores AS (
                SELECT photo_id, COUNT(DISTINCT person_id) AS match_count
                FROM photobase.photo_regions
                WHERE person_id IN ({$personList})
                GROUP BY photo_id
            )
        ";
    }
}

$matchCountExpr = $needsScores ? "ps.match_count" : "0";
$scoresJoin     = $needsScores ? "JOIN photo_scores ps ON ps.photo_id = pi.id" : "";

$sql = $scoresCte . "
    SELECT pi.id, pi.file, pi.path, pi.media_name, pi.date_creation, pi.date_available, {$matchCountExpr} AS match_count
    FROM photobase.photo_images pi
    {$scoresJoin}
    WHERE pi.missing_since IS NULL
";

if ($mediaEnabled && !empty($media)) {
    $sql .= " AND pi.media_name IN (" . implode(",", $mediaPlaceholders) . ")";
}

$sql .= " AND (pi.ext='jpg' OR pi.ext='jpeg')";

if ($dateEnabled) {
    $sql .= " AND COALESCE(pi.date_creation, pi.date_available) BETWEEN :date_from AND :date_to";
}

$sql .= ($sortBy === "face_match_count")
    ? " ORDER BY match_count DESC, COALESCE(pi.date_creation, pi.date_available) DESC"
    : " ORDER BY COALESCE(pi.date_creation, pi.date_available) DESC";

if ($limitEnabled) {
    $sql .= " LIMIT :max_count";
}

try {
    $stmt = $db->prepare($sql);

    if ($needsScores) {
        foreach ($personIds as $i => $pid) {
            $stmt->bindValue(":person{$i}", $pid, PDO::PARAM_INT);
        }
        if ($facesUseDescriptor) {
            $stmt->bindValue(":threshold", $similarityThreshold);
        }
    }

    if ($mediaEnabled) {
        foreach ($media as $i => $m) {
            $stmt->bindValue(":media{$i}", $m, PDO::PARAM_STR);
        }
    }

    if ($dateEnabled) {
        $stmt->bindValue(":date_from", $dateFrom);
        $stmt->bindValue(":date_to", $dateTo);
    }

    if ($limitEnabled) {
        $stmt->bindValue(":max_count", $maxCount, PDO::PARAM_INT);
    }

    $stmt->execute();

    $result = $stmt->fetchAll(PDO::FETCH_ASSOC);

    echo json_encode($result);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}