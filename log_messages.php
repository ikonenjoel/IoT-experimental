<?php
header('Content-Type: application/json');

// Only allow POST requests
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['status' => 'error', 'message' => 'Method Not Allowed']);
    exit;
}

// Authorized UID whitelist. Currnetly only has one authorized UID, need more tags.
$authorized_uids = [
    '3DCEEA53'
];

$uid = isset($_POST['uid']) ? strtoupper(trim($_POST['uid'])) : '';

if (empty($uid)) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'UID missing']);
    exit;
}

// Optional: Log access attempt to a local text file
$log_entry = date('Y-m-d H:i:s') . " - UID: " . $uid;

if (in_array($uid, $authorized_uids, true)) {
    file_put_contents('access.log', $log_entry . " - ACCEPTED\n", FILE_APPEND);
    
    echo json_encode([
        'status'  => 'accepted',
        'message' => 'Access Granted'
    ]);
} else {
    file_put_contents('access.log', $log_entry . " - DENIED\n", FILE_APPEND);
    
    echo json_encode([
        'status'  => 'denied',
        'message' => 'Access Denied'
    ]);
}
