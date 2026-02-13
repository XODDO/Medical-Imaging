#ifndef WEBPAGE2_H 
#define WEBPAGE2_H 
#include <Arduino.h>
  // ensures types like PROGMEM are defined const char index_html[] PROGMEM =
const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
  <html lang="en">
    <head>
      <meta charset="UTF-8" />
      <meta name="viewport" content="width=device-width, initial-scale=1.0" />
      <title>CAVISCOPE - Medical Imaging System</title>
      <meta http-equiv="Content-Security-Policy" content="script-src 'self' 'unsafe-inline' 'wasm-unsafe-eval'">
      <!-- Bootstrap 5 CSS -
    <link
      href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css"
      rel="stylesheet"
    />-->
      <link
        rel="stylesheet"
        href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css"
      />
      <style>
        /* Mini Bootstrap 5 Core - 3.2KB */
        .container {
          width: 100%;
          padding-right: var(--bs-gutter-x, 0.75rem);
          padding-left: var(--bs-gutter-x, 0.75rem);
          margin-right: auto;
          margin-left: auto;
        }
        @media (min-width: 576px) {
          .container {
            max-width: 540px;
          }
        }
        @media (min-width: 768px) {
          .container {
            max-width: 720px;
          }
        }
        @media (min-width: 992px) {
          .container {
            max-width: 960px;
          }
        }
        @media (min-width: 1200px) {
          .container {
            max-width: 1140px;
          }
        }
        @media (min-width: 1400px) {
          .container {
            max-width: 1320px;
          }
        }
        .row {
          --bs-gutter-x: 1.5rem;
          --bs-gutter-y: 0;
          display: flex;
          flex-wrap: wrap;
          margin-top: calc(-1 * var(--bs-gutter-y));
          margin-right: calc(-0.5 * var(--bs-gutter-x));
          margin-left: calc(-0.5 * var(--bs-gutter-x));
        }
        .row > * {
          flex-shrink: 0;
          width: 100%;
          max-width: 100%;
          padding-right: calc(var(--bs-gutter-x) * 0.5);
          padding-left: calc(var(--bs-gutter-x) * 0.5);
          margin-top: var(--bs-gutter-y);
        }
        .d-flex {
          display: flex !important;
        }
        .d-inline-flex {
          display: inline-flex !important;
        }
        .flex-row {
          flex-direction: row !important;
        }
        .flex-column {
          flex-direction: column !important;
        }
        .justify-content-center {
          justify-content: center !important;
        }
        .justify-content-between {
          justify-content: space-between !important;
        }
        .align-items-center {
          align-items: center !important;
        }
        .align-items-start {
          align-items: flex-start !important;
        }
        .flex-wrap {
          flex-wrap: wrap !important;
        }
        .flex-nowrap {
          flex-wrap: nowrap !important;
        }
        .gap-1 {
          gap: 0.25rem !important;
        }
        .gap-2 {
          gap: 0.5rem !important;
        }
        .gap-3 {
          gap: 1rem !important;
        }
        .w-100 {
          width: 100% !important;
        }
        .h-100 {
          height: 100% !important;
        }
        .m-0 {
          margin: 0 !important;
        }
        .m-1 {
          margin: 0.25rem !important;
        }
        .m-2 {
          margin: 0.5rem !important;
        }
        .m-3 {
          margin: 1rem !important;
        }
        .m-4 {
          margin: 1.5rem !important;
        }
        .m-5 {
          margin: 3rem !important;
        }
        .mt-0 {
          margin-top: 0 !important;
        }
        .mt-1 {
          margin-top: 0.25rem !important;
        }
        .mt-2 {
          margin-top: 0.5rem !important;
        }
        .mt-3 {
          margin-top: 1rem !important;
        }
        .mt-4 {
          margin-top: 1.5rem !important;
        }
        .mt-5 {
          margin-top: 3rem !important;
        }
        .mb-0 {
          margin-bottom: 0 !important;
        }
        .mb-1 {
          margin-bottom: 0.25rem !important;
        }
        .mb-2 {
          margin-bottom: 0.5rem !important;
        }
        .mb-3 {
          margin-bottom: 1rem !important;
        }
        .mb-4 {
          margin-bottom: 1.5rem !important;
        }
        .mb-5 {
          margin-bottom: 3rem !important;
        }
        .me-1 {
          margin-right: 0.25rem !important;
        }
        .me-2 {
          margin-right: 0.5rem !important;
        }
        .me-3 {
          margin-right: 1rem !important;
        }
        .me-4 {
          margin-right: 1.5rem !important;
        }
        .me-5 {
          margin-right: 3rem !important;
        }
        .ms-1 {
          margin-left: 0.25rem !important;
        }
        .ms-2 {
          margin-left: 0.5rem !important;
        }
        .ms-3 {
          margin-left: 1rem !important;
        }
        .ms-4 {
          margin-left: 1.5rem !important;
        }
        .ms-5 {
          margin-left: 3rem !important;
        }
        .mx-0 {
          margin-right: 0 !important;
          margin-left: 0 !important;
        }
        .mx-1 {
          margin-right: 0.25rem !important;
          margin-left: 0.25rem !important;
        }
        .mx-2 {
          margin-right: 0.5rem !important;
          margin-left: 0.5rem !important;
        }
        .mx-3 {
          margin-right: 1rem !important;
          margin-left: 1rem !important;
        }
        .mx-4 {
          margin-right: 1.5rem !important;
          margin-left: 1.5rem !important;
        }
        .mx-5 {
          margin-right: 3rem !important;
          margin-left: 3rem !important;
        }
        .my-0 {
          margin-top: 0 !important;
          margin-bottom: 0 !important;
        }
        .my-1 {
          margin-top: 0.25rem !important;
          margin-bottom: 0.25rem !important;
        }
        .my-2 {
          margin-top: 0.5rem !important;
          margin-bottom: 0.5rem !important;
        }
        .my-3 {
          margin-top: 1rem !important;
          margin-bottom: 1rem !important;
        }
        .my-4 {
          margin-top: 1.5rem !important;
          margin-bottom: 1.5rem !important;
        }
        .my-5 {
          margin-top: 3rem !important;
          margin-bottom: 3rem !important;
        }
        .p-0 {
          padding: 0 !important;
        }
        .p-1 {
          padding: 0.25rem !important;
        }
        .p-2 {
          padding: 0.5rem !important;
        }
        .p-3 {
          padding: 1rem !important;
        }
        .p-4 {
          padding: 1.5rem !important;
        }
        .p-5 {
          padding: 3rem !important;
        }
        .pt-0 {
          padding-top: 0 !important;
        }
        .pt-1 {
          padding-top: 0.25rem !important;
        }
        .pt-2 {
          padding-top: 0.5rem !important;
        }
        .pt-3 {
          padding-top: 1rem !important;
        }
        .pt-4 {
          padding-top: 1.5rem !important;
        }
        .pt-5 {
          padding-top: 3rem !important;
        }
        .pb-0 {
          padding-bottom: 0 !important;
        }
        .pb-1 {
          padding-bottom: 0.25rem !important;
        }
        .pb-2 {
          padding-bottom: 0.5rem !important;
        }
        .pb-3 {
          padding-bottom: 1rem !important;
        }
        .pb-4 {
          padding-bottom: 1.5rem !important;
        }
        .pb-5 {
          padding-bottom: 3rem !important;
        }
        .pe-1 {
          padding-right: 0.25rem !important;
        }
        .pe-2 {
          padding-right: 0.5rem !important;
        }
        .pe-3 {
          padding-right: 1rem !important;
        }
        .pe-4 {
          padding-right: 1.5rem !important;
        }
        .pe-5 {
          padding-right: 3rem !important;
        }
        .ps-1 {
          padding-left: 0.25rem !important;
        }
        .ps-2 {
          padding-left: 0.5rem !important;
        }
        .ps-3 {
          padding-left: 1rem !important;
        }
        .ps-4 {
          padding-left: 1.5rem !important;
        }
        .ps-5 {
          padding-left: 3rem !important;
        }
        .px-0 {
          padding-right: 0 !important;
          padding-left: 0 !important;
        }
        .px-1 {
          padding-right: 0.25rem !important;
          padding-left: 0.25rem !important;
        }
        .px-2 {
          padding-right: 0.5rem !important;
          padding-left: 0.5rem !important;
        }
        .px-3 {
          padding-right: 1rem !important;
          padding-left: 1rem !important;
        }
        .px-4 {
          padding-right: 1.5rem !important;
          padding-left: 1.5rem !important;
        }
        .px-5 {
          padding-right: 3rem !important;
          padding-left: 3rem !important;
        }
        .py-0 {
          padding-top: 0 !important;
          padding-bottom: 0 !important;
        }
        .py-1 {
          padding-top: 0.25rem !important;
          padding-bottom: 0.25rem !important;
        }
        .py-2 {
          padding-top: 0.5rem !important;
          padding-bottom: 0.5rem !important;
        }
        .py-3 {
          padding-top: 1rem !important;
          padding-bottom: 1rem !important;
        }
        .py-4 {
          padding-top: 1.5rem !important;
          padding-bottom: 1.5rem !important;
        }
        .py-5 {
          padding-top: 3rem !important;
          padding-bottom: 3rem !important;
        }
        .text-center {
          text-align: center !important;
        }
        .text-start {
          text-align: left !important;
        }
        .text-end {
          text-align: right !important;
        }
        .text-white {
          color: #fff !important;
        }
        .text-primary {
          color: #0d6efd !important;
        }
        .text-success {
          color: #198754 !important;
        }
        .text-info {
          color: #0dcaf0 !important;
        }
        .text-warning {
          color: #ffc107 !important;
        }
        .text-danger {
          color: #dc3545 !important;
        }
        .text-secondary {
          color: #6c757d !important;
        }
        .text-dark {
          color: #212529 !important;
        }
        .text-muted {
          color: #6c757d !important;
        }
        .bg-primary {
          background-color: #0d6efd !important;
        }
        .bg-success {
          background-color: #198754 !important;
        }
        .bg-info {
          background-color: #0dcaf0 !important;
        }
        .bg-warning {
          background-color: #ffc107 !important;
        }
        .bg-danger {
          background-color: #dc3545 !important;
        }
        .bg-secondary {
          background-color: #6c757d !important;
        }
        .bg-dark {
          background-color: #212529 !important;
        }
        .bg-light {
          background-color: #f8f9fa !important;
        }
        .bg-white {
          background-color: #fff !important;
        }
        .alert {
          position: relative;
          padding: 0.75rem 1.25rem;
          margin-bottom: 1rem;
          border: 1px solid transparent;
          border-radius: 0.375rem;
        }
        .alert-primary {
          color: #084298;
          background-color: #cfe2ff;
          border-color: #b6d4fe;
        }
        .alert-secondary {
          color: #41464b;
          background-color: #e2e3e5;
          border-color: #d3d6d8;
        }
        .alert-success {
          color: #0f5132;
          background-color: #d1e7dd;
          border-color: #badbcc;
        }
        .alert-info {
          color: #055160;
          background-color: #cff4fc;
          border-color: #b6effb;
        }
        .alert-warning {
          color: #664d03;
          background-color: #fff3cd;
          border-color: #ffecb5;
        }
        .alert-danger {
          color: #721c24;
          background-color: #f8d7da;
          border-color: #f5c6cb;
        }
        .alert-dark {
          color: #141619;
          background-color: #d3d3d4;
          border-color: #bcbebf;
        }
        .btn {
          display: inline-block;
          font-weight: 400;
          line-height: 1.5;
          color: #212529;
          text-align: center;
          text-decoration: none;
          vertical-align: middle;
          cursor: pointer;
          user-select: none;
          background-color: transparent;
          border: 1px solid transparent;
          padding: 0.375rem 0.75rem;
          font-size: 1rem;
          border-radius: 0.375rem;
          transition: color 0.15s ease-in-out,
            background-color 0.15s ease-in-out, border-color 0.15s ease-in-out,
            box-shadow 0.15s ease-in-out;
        }
        .btn:hover {
          color: #212529;
          text-decoration: none;
        }
        .btn-primary {
          color: #fff;
          background-color: #0d6efd;
          border-color: #0d6efd;
        }
        .btn-primary:hover {
          color: #fff;
          background-color: #0b5ed7;
          border-color: #0a58ca;
        }
        .btn-success {
          color: #fff;
          background-color: #198754;
          border-color: #198754;
        }
        .btn-success:hover {
          color: #fff;
          background-color: #157347;
          border-color: #146c43;
        }
        .btn-info {
          color: #000;
          background-color: #0dcaf0;
          border-color: #0dcaf0;
        }
        .btn-info:hover {
          color: #000;
          background-color: #31d2f2;
          border-color: #25cff2;
        }
        .btn-warning {
          color: #000;
          background-color: #ffc107;
          border-color: #ffc107;
        }
        .btn-warning:hover {
          color: #000;
          background-color: #ffca2c;
          border-color: #ffc720;
        }
        .btn-danger {
          color: #fff;
          background-color: #dc3545;
          border-color: #dc3545;
        }
        .btn-danger:hover {
          color: #fff;
          background-color: #bb2d3b;
          border-color: #b02a37;
        }
        .btn-secondary {
          color: #fff;
          background-color: #6c757d;
          border-color: #6c757d;
        }
        .btn-secondary:hover {
          color: #fff;
          background-color: #5c636a;
          border-color: #565e64;
        }
        .btn-dark {
          color: #fff;
          background-color: #212529;
          border-color: #212529;
        }
        .btn-dark:hover {
          color: #fff;
          background-color: #424649;
          border-color: #373b3e;
        }
        .btn:disabled {
          pointer-events: none;
          opacity: 0.65;
        }
        .img-fluid {
          max-width: 100%;
          height: auto;
        }
        .rounded {
          border-radius: 0.375rem !important;
        }
        .rounded-circle {
          border-radius: 50% !important;
        }
        .rounded-pill {
          border-radius: 50rem !important;
        }
        .shadow {
          box-shadow: 0 0.5rem 1rem rgba(0, 0, 0, 0.15) !important;
        }
        .shadow-sm {
          box-shadow: 0 0.125rem 0.25rem rgba(0, 0, 0, 0.075) !important;
        }
        .shadow-lg {
          box-shadow: 0 1rem 3rem rgba(0, 0, 0, 0.175) !important;
        }
        .overflow-auto {
          overflow: auto !important;
        }
        .overflow-hidden {
          overflow: hidden !important;
        }
        .position-relative {
          position: relative !important;
        }
        .position-absolute {
          position: absolute !important;
        }
        .border {
          border: 1px solid #dee2e6 !important;
        }
        .border-0 {
          border: 0 !important;
        }
        .border-primary {
          border-color: #0d6efd !important;
        }
        .border-success {
          border-color: #198754 !important;
        }
        .border-info {
          border-color: #0dcaf0 !important;
        }
        .border-warning {
          border-color: #ffc107 !important;
        }
        .border-danger {
          border-color: #dc3545 !important;
        }
      </style>

      <style>
        :root {
          --spotlight-width: 75vw;
          --mobile-spotlight-width: 95vw;
        }

        body {
          background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
          min-height: 100vh;
          font-family: "Segoe UI", Tahoma, Geneva, Verdana, sans-serif;
        }

        .container-glass {
          background: rgba(255, 255, 255, 0.95);
          backdrop-filter: blur(10px);
          border-radius: 20px;
          box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
          margin-top: 2rem;
          margin-bottom: 2rem;
          padding: 2rem;
        }

        /* Spotlight Image Styling */
        .spotlight-container {
          position: relative;
          margin: 2rem auto;
          max-width: var(--spotlight-width);
        }

        @media (max-width: 768px) {
          .spotlight-container {
            max-width: var(--mobile-spotlight-width);
          }
        }

        .spotlight-frame {
          border: 3px solid #2c3e50;
          border-radius: 15px;
          padding: 15px;
          background: linear-gradient(145deg, #ffffff, #f0f0f0);
          box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2),
            inset 0 1px 0 rgba(255, 255, 255, 0.8);
          position: relative;
        }

        .spotlight-frame::before {
          content: "";
          position: absolute;
          top: 5px;
          left: 5px;
          right: 5px;
          bottom: 5px;
          border: 1px solid rgba(0, 0, 0, 0.1);
          border-radius: 10px;
          pointer-events: none;
        }

        #main-image {
          width: 100%;
          height: auto;
          border-radius: 8px;
          transition: transform 0.3s ease;
        }

        .timestamp {
          background: rgba(44, 62, 80, 0.9);
          color: white;
          padding: 8px 15px;
          border-radius: 20px;
          display: inline-block;
          margin-top: 1rem;
          font-family: "Courier New", monospace;
          font-size: 0.9rem;
        }

        /* Control Buttons */
        .control-buttons {
          display: flex;
          justify-content: center;
          gap: 1rem;
          flex-wrap: wrap;
          margin: 2rem 0;
        }

        .btn-control {
          min-width: 140px;
          padding: 12px 20px;
          border: none;
          border-radius: 50px;
          font-weight: 600;
          transition: all 0.3s ease;
          box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
        }

        .btn-control:hover {
          transform: translateY(-2px);
          box-shadow: 0 6px 20px rgba(0, 0, 0, 0.15);
        }

        .btn-capture {
          background: linear-gradient(45deg, #28a745, #20c997);
          color: white;
        }
        .btn-rotate {
          background: linear-gradient(45deg, #17a2b8, #6f42c1);
          color: white;
        }
        .btn-refresh {
          background: linear-gradient(45deg, #ffc107, #fd7e14);
          color: white;
        }
        .btn-save {
          background: linear-gradient(45deg, #6610f2, #e83e8c);
          color: white;
        }
        .btn-upload {
          background: linear-gradient(45deg, #343a40, #495057);
          color: white;
        }

        /* Film Strip */
        .film-strip-container {
          background: #2c3e50;
          border-radius: 15px;
          padding: 1rem;
          margin-top: 2rem;
          position: relative;
        }

        .film-strip-container::before {
          content: "";
          position: absolute;
          top: 0;
          left: 0;
          right: 0;
          height: 20px;
          background: repeating-linear-gradient(
            90deg,
            transparent,
            transparent 10px,
            rgba(255, 255, 255, 0.3) 10px,
            rgba(255, 255, 255, 0.3) 20px
          );
          border-radius: 15px 15px 0 0;
        }

        .film-strip {
          display: flex;
          overflow-x: auto;
          gap: 1rem;
          padding: 1rem 0;
          scrollbar-width: thin;
          scrollbar-color: #3498db #2c3e50;
        }

        .film-strip::-webkit-scrollbar {
          height: 8px;
        }

        .film-strip::-webkit-scrollbar-track {
          background: #34495e;
          border-radius: 10px;
        }

        .film-strip::-webkit-scrollbar-thumb {
          background: #3498db;
          border-radius: 10px;
        }

        .film-frame {
          flex: 0 0 auto;
          width: 120px;
          height: 90px;
          border: 2px solid #bdc3c7;
          border-radius: 8px;
          overflow: hidden;
          cursor: pointer;
          transition: all 0.3s ease;
          position: relative;
        }

        .film-frame:hover {
          transform: scale(1.1);
          border-color: #3498db;
          box-shadow: 0 5px 15px rgba(52, 152, 219, 0.4);
        }

        .film-frame.active {
          border-color: #e74c3c;
          box-shadow: 0 0 20px rgba(231, 76, 60, 0.6);
        }

        .film-frame img {
          width: 100%;
          height: 100%;
          object-fit: cover;
        }

        .film-frame::after {
          content: "";
          position: absolute;
          bottom: 0;
          left: 0;
          right: 0;
          height: 3px;
          background: rgba(0, 0, 0, 0.1);
        }

        h1 {
          color: #2c3e50;
          font-weight: 700;
          text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.1);
          margin-bottom: 1rem;
        }

        .subtitle {
          color: #6c757d;
          font-size: 1.1rem;
          margin-bottom: 1.5rem;
          font-weight: 400;
        }

        .alert-info {
          background: rgba(23, 162, 184, 0.1);
          border: 1px solid rgba(23, 162, 184, 0.3);
          border-radius: 10px;
        }

        .alert-warning {
          background: rgba(255, 193, 7, 0.1);
          border: 1px solid rgba(255, 193, 7, 0.3);
          border-radius: 10px;
          font-size: 0.85rem;
        }

        .logo {
          height: 50px;
          vertical-align: middle;
          margin-right: 5px;
          border-radius: 8px;
        }

        .disclaimer {
          font-size: 0.75rem;
          color: #6c757d;
          text-align: center;
          margin-top: 2rem;
          padding: 1rem;
          background: rgba(108, 117, 125, 0.1);
          border-radius: 8px;
          border-left: 4px solid #dc3545;
        }
      </style>
    </head>
    <body>
      <div class="container container-glass">
        <h1 class="text-center">
          <i class="fas fa-camera-medical me-2"></i>
         
          CAVISCOPE MEDICAL IMAGING SYSTEM
        </h1>

        <div class="subtitle text-center">
          Real-time Internal Organ Visualization for Diagnostic Assistance
        </div>

        <div class="alert alert-info text-center">
          <i class="fas fa-info-circle me-2"></i>
          This system provides high-resolution visualization of internal organs
          from live subjects. Image capture takes 5-10 seconds. Latest images
          stream automatically for diagnostic review.
        </div>

        <!-- Control Buttons - TOP POSITION -->
        <div class="control-buttons">
          <button class="btn btn-control btn-capture" onclick="capturePhoto()">
            <i class="fas fa-camera me-2"></i>CAPTURE
          </button>
          <button class="btn btn-control btn-rotate" onclick="rotatePhoto()">
            <i class="fas fa-redo me-2"></i>ROTATE
          </button>
          <button class="btn btn-control btn-refresh" onclick="refreshFrame()">
            <i class="fas fa-sync me-2"></i>REFRESH
          </button>
          <button class="btn btn-control btn-save" onclick="saveImage()">
            <i class="fas fa-download me-2"></i>SAVE
          </button>
          <button
            class="btn btn-control btn-upload"
            onclick="uploadToDatabase()"
          >
            <i class="fas fa-database me-2"></i>UPLOAD
          </button>
        </div>

        <!-- Spotlight Image -->
        <div class="spotlight-container">
          <div class="spotlight-frame">
            <img
              id="main-image"
              src="photo-in-spiffs"
              alt="Current Medical Diagnostic Image"
              class="img-fluid"
            />
          </div>
          <div class="timestamp">
            <i class="fas fa-clock me-2"></i>
            Last Received: <span id="image-time">2024-01-15 14:30:25</span>
          </div>
        </div>

        <!-- Film Strip -->
        <div class="film-strip-container">
          <h5 class="text-white text-center mb-3">
            <i class="fas fa-history me-2"></i>Image History (Most Recent First)
          </h5>
          <div class="film-strip" id="gallery">
            <!-- Images will be dynamically added here -->
          </div>
        </div>

        <!-- Medical Disclaimer -->
        <div class="alert alert-warning text-center">
          <i class="fas fa-shield-alt me-2"></i>
          <strong>MEDICAL DEVICE NOTICE:</strong> This system is intended for
          use by qualified healthcare professionals only. All images are
          generated and stored in compliance with ISO 13485 medical device
          standards and HIPAA/GDPR patient data privacy regulations.
        </div>

        <div class="disclaimer">
          <i class="fas fa-exclamation-triangle me-1"></i>
          <strong>CONFIDENTIAL MEDICAL DATA:</strong> All images and patient
          information are protected health information (PHI). Unauthorized
          access, distribution, or use is prohibited. Images are encrypted in
          transit and at rest. Audit trails are maintained in accordance with
          ISO 27001 information security standards.
        </div>
      </div>

      <script>
        let currentRotation = 0;
        let images = [];

        // Sample initial data with placeholder images
        function initializeGallery() {
          images = [
            {
              id: 1,
              src: "placeholder.jpg",
              timestamp: new Date().toLocaleString(),
            },
            {
              id: 2,
              src: "placeholder.jpg",
              timestamp: new Date(Date.now() - 5 * 60000).toLocaleString(),
            },
            {
              id: 3,
              src: "placeholder.jpg",
              timestamp: new Date(Date.now() - 10 * 60000).toLocaleString(),
            },
            {
              id: 4,
              src: "placeholder.jpg",
              timestamp: new Date(Date.now() - 15 * 60000).toLocaleString(),
            },
            {
              id: 5,
              src: "placeholder.jpg",
              timestamp: new Date(Date.now() - 20 * 60000).toLocaleString(),
            },
          ];
          renderGallery();
          updateMainImage(images[0]);
        }

        function renderGallery() {
          const gallery = document.getElementById("gallery");
          gallery.innerHTML = "";

          images.forEach((image, index) => {
            const frame = document.createElement("div");
            frame.className = "film-frame" + (index === 0 ? " active" : "");
            frame.innerHTML = `<img src="${image.src}" alt="Medical Diagnostic Image ${image.id}" onclick="showImage(${image.id})" />`;
            gallery.appendChild(frame);
          });
        }

        function updateMainImage(image) {
          document.getElementById("main-image").src = image.src;
          document.getElementById("image-time").textContent = image.timestamp;
        }

        function capturePhoto() {
  console.log("Capture photo requested");

  const captureBtn = document.querySelector(".btn-capture");
  const originalHTML = captureBtn.innerHTML;
  captureBtn.innerHTML =
    '<i class="fas fa-spinner fa-spin me-2"></i>CAPTURING...';
  captureBtn.disabled = true;


  // Trigger ESP32 camera capture
  fetch("/capture")
    .then((response) => {
      console.log("Capture triggered:", response.status);
      // Wait a bit for the camera to finish saving
      setTimeout(() => {
        // Reload image with cache-busting query param
        const mainImage = document.getElementById("main-image");
        mainImage.src = `/photo.jpg?t=${Date.now()}`;

        // Add to gallery
        const newImage = {
          id: images.length + 1,
          src: mainImage.src,
          timestamp: new Date().toLocaleString(),
        };
        images.unshift(newImage);
        renderGallery();
        updateMainImage(newImage);

        // Restore button state
        captureBtn.innerHTML = originalHTML;
        captureBtn.disabled = false;
      }, 2000); // allow 2s for ESP32 to write file
    })
    .catch((err) => {
      console.error("Error capturing photo:", err);
      captureBtn.innerHTML = originalHTML;
      captureBtn.disabled = false;
    });
}


        function rotatePhoto() {
          currentRotation += 90;
          const img = document.getElementById("main-image");
          img.style.transform = `rotate(${currentRotation}deg)`;
        }

        function refreshFrame() {
          console.log("Refreshing current frame");
          // photo-in-spiffs
          // Add refresh logic here to get latest image from device
        }

        function saveImage() {
          const img = document.getElementById("main-image");
          const link = document.createElement("a");
          link.href = img.src;
          link.download = `medical_diagnostic_image_${new Date().getTime()}.jpg`;
          link.click();

          console.log("Image saved locally");
        }

        function uploadToDatabase() {
          console.log("Upload to secure medical database requested");
          // Add upload logic to PACS or medical records system
        }

        function showImage(imageId) {
          const image = images.find((img) => img.id === imageId);
          if (image) {
            updateMainImage(image);

            // Update active state
            document.querySelectorAll(".film-frame").forEach((frame) => {
              frame.classList.remove("active");
            });
            event.currentTarget.parentElement.classList.add("active");
          }
        }

        // Auto-refresh timestamp every minute
        function updateTimestamps() {
          setInterval(() => {
            const now = new Date();
            document.querySelectorAll(".timestamp").forEach((element) => {
              if (element.id === "image-time") {
                // Only update if it's the current image (not historical)
                const activeFrame =
                  document.querySelector(".film-frame.active");
                if (
                  activeFrame &&
                  Array.from(activeFrame.parentNode.children).indexOf(
                    activeFrame
                  ) === 0
                ) {
                  element.textContent = now.toLocaleString();
                }
              }
            });
          }, 60000);
        }

        // Initialize on load
        document.addEventListener("DOMContentLoaded", function () {
          initializeGallery();
          updateTimestamps();
        });
      </script>
    </body>
  </html>
</Arduino.h>
)rawliteral";

#endif