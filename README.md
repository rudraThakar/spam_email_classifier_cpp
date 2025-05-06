# 📧 Spam/Ham Email Classifier using C++ and Hash Maps

A lightweight, fast, and explainable email spam classifier implemented in C++. The project utilizes custom-built **hash maps** to store word frequencies and applies a **probabilistic scoring algorithm** for real-time spam detection. A user-friendly **GTK-based GUI** allows users to classify emails and provide feedback for adaptive learning.

---

## 🧠 Overview

Spam detection plays a critical role in digital communication. Our solution focuses on a **resource-efficient approach** using basic data structures instead of complex machine learning models. The classifier analyzes the frequency of words like “lottery” or “prize” (commonly found in spam) to compute a spam probability score.

---

## ⚙️ How It Works

- 📊 **Dataset**: A CSV file containing the frequency of each word in both spam and ham (non-spam) emails.
- 🗃️ **Custom Hash Maps**: 
  - **Chaining Hash Map**: Handles collisions with linked lists.
  - **Open Addressing Hash Map**: Uses linear probing for efficient lookups.
- 🧮 **Spam Score Computation**:
  - The spam score of an email is calculated as the average of the individual spam probabilities of its words.
- 🖥️ **GTK GUI Interface**:
  - Enter email content
  - Classify as spam or ham
  - Provide feedback to update word frequencies

---


## 🧱 Core Components

- **ChainingHashMap** and **OpenAddressingHashMap** for flexible and efficient frequency storage.
- **Probabilistic scoring function** to calculate spam likelihood based on word usage.
- **GTK GUI** for user interaction, highlighting:
  - Email input
  - One-click spam/ham classification
  - Feedback integration to improve future predictions

---

## 🖼️ GUI Features

- 🔍 **Email Classification**: Enter or paste email content to classify.
- 🧠 **Adaptive Learning**: Mark classification as correct or incorrect to update training data.
- 🖍️ **Visual Word-Level Highlighting** *(optional)*: See which words contributed most to the score.

---
